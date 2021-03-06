//
// Created by Hao Xu on 16/8/11.
//

#include "RapidFDM/simulation/simulation_dji_a3_adapter.h"



namespace RapidFDM {
    namespace Simulation {
        simulation_dji_a3_adapter::simulation_dji_a3_adapter(SimulatorAircraft *_sim_air) :
                simulation_hil_adapter(_sim_air) {
            root_uri = "ws://127.0.0.1:19870/general";
            sim_uri = "ws://127.0.0.1:19870/controller/simulator/";

//            root_uri = "ws://10.211.55.3:19870/general";
//            sim_uri = "ws://10.211.55.3:19870/controller/simulator/";
            c_root.set_access_channels(websocketpp::log::alevel::none);
            c_root.clear_access_channels(websocketpp::log::alevel::all);

            c_root.init_asio(&io_service);

            c_root.set_message_handler(bind(&simulation_dji_a3_adapter::on_message_root, this, &c_root, ::_1, ::_2));
            c_root.set_fail_handler(bind(&simulation_dji_a3_adapter::on_assitant_failed, this, &c_root, ::_1));
            c_root.set_close_handler(bind(&simulation_dji_a3_adapter::on_assitant_failed, this, &c_root, ::_1));

            c_root.set_open_handler(bind(&simulation_dji_a3_adapter::on_assitant_open, this, &c_root, ::_1));

            try_connect_assistant();



        }

        void simulation_dji_a3_adapter::on_message_root(client *c, websocketpp::connection_hdl hdl, message_ptr msg) {

            rapidjson::Document d;
            d.Parse(msg->get_payload().c_str());
            if (!d.IsObject()) {
                std::wcerr << "Parse a3 assiant document failed!" << std::endl;
                std::cout << msg->get_payload();
                return;
            }

            if (!d.HasMember("FILE") || !d["FILE"].IsString() || !d.HasMember("EVENT")) {
                return;
            }

            if (std::string(d["EVENT"].GetString()) != "device_arrival") {
                std::cout << msg->get_payload() << std::endl;
                return;
            } else {
                printf("Start connect simulator\n");
            }

            file_name = d["FILE"].GetString();
            try_connect_simulator();
        }

        void simulation_dji_a3_adapter::try_connect_simulator() {
            std::cout << "Try to connect simulator on " << file_name << std::endl;

            std::string uri = sim_uri + file_name;
            websocketpp::lib::error_code ec;
            std::cout << uri << std::endl;
            if (c_simulator != nullptr) {
                c_simulator->close(sim_connection_hdl, websocketpp::close::status::normal, "Close connect");
            }
            c_simulator = new client;
            c_simulator->set_access_channels(websocketpp::log::alevel::none);
            c_simulator->clear_access_channels(websocketpp::log::alevel::all);
            c_simulator->init_asio(&c_root.get_io_service());

            c_simulator->set_message_handler(
                    bind(&simulation_dji_a3_adapter::on_message_simulator, this, c_simulator, ::_1, ::_2));
            c_simulator->set_open_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_open, this, c_simulator, ::_1));

            c_simulator->set_close_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_failed, this, c_simulator, ::_1));

            c_simulator->set_fail_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_closed, this, c_simulator, ::_1));

            client::connection_ptr con = c_simulator->get_connection(uri, ec);
            if (ec) {
                std::cerr << "could not create connection to Simulator because: " << ec.message() << std::endl;
                return;
            }

            c_simulator->connect(con);

        }

        void simulation_dji_a3_adapter::on_simulator_link_open(client *c, websocketpp::connection_hdl hdl) {
            rapidjson::Document d;
            d.SetObject();
            d.AddMember("SEQ", "FUCK", d.GetAllocator());
            d.AddMember("CMD", "start_sim", d.GetAllocator());
            d.AddMember("latitude", 0, d.GetAllocator());
            d.AddMember("longitude", 1, d.GetAllocator());
            d.AddMember("frequency", 50, d.GetAllocator());
            d.AddMember("only_aircraft", 1, d.GetAllocator());
            sim_connection_hdl = hdl;
            int32_t size;
            const char *str = json_to_buffer(d, size);
            c->send(hdl, str, size, websocketpp::frame::opcode::text);

            printf("Simulator %s Open", file_name.c_str());

        }

        void simulation_dji_a3_adapter::tick_func(float dt,long tick) {
            this->simulator_tick = tick;
            if (total_tick_count % 200 == 1) {
                try_connect_assistant();
                printf("time: %f d tick %d\n", total_tick_count / 200.0, dcount);
                dcount = 0;
            }
            this->send_realtime_data();
        }

        void simulation_dji_a3_adapter::on_message_simulator(client *c, websocketpp::connection_hdl hdl,
                                                             message_ptr msg) {
            rapidjson::Document d;
            d.Parse(msg->get_payload().c_str());

            assert(d.IsObject());
            this->sim_online = true;
            if (fast_string(d, "EVENT") == "sim_state") {
                motor_starter = fast_value(d, "MotorStarted") > 0;
                RcA = fast_value(d, "RcA");
                RcE = fast_value(d, "RcE");
                RcR = fast_value(d, "RcR");
                RcT = (fast_value(d, "RcT"))/2 + 5000;
                return;
            } else if (fast_string(d, "EVENT") == "sim_pwm") {
                /*
                 * stdout: {
                   "EVENT": "sim_pwm",
                   "channels": [....    ],
                    "tick": 60
                 */

                dcount++;
                if (total_tick_count % 200 == 1) {
                    printf("tick latency %ld sim:%ld fc %ld\n", this->simulator_tick - (int) (fast_value(d, "tick")),
                           this->simulator_tick, (long) (fast_value(d, "tick"))
                    );
                }

                for (int chn = 0; chn < 8; chn++) {
                    pwm[chn] = d["channels"][chn].GetDouble();
                    pwm[chn] = (pwm[chn] / 10000 - 0.5) * 2;
                    pwm[chn] = float_constrain(pwm[chn], -1, 1);
                }
                on_pwm_data_receieve(pwm, 8);
            }
            else
            {
                std::cout << msg->get_payload() << std::endl;
            }

            if (on_receive_pwm != nullptr) {
                (*on_receive_pwm)();
            }
        }

        void simulation_dji_a3_adapter::push_json_to_app(rapidjson::Document &d) {
            rapidjson::Value a3_value(rapidjson::kObjectType);
            add_value(a3_value, motor_starter, d, "motor_started");
            add_value(a3_value, RcA, d, "RcA");
            add_value(a3_value, RcE, d, "RcE");
            add_value(a3_value, RcR, d, "RcR");
            add_value(a3_value, RcT, d, "RcT");
            add_value(a3_value, (int) sim_online, d, "online");
            add_value(a3_value, 1, d, "sim_mode");

            rapidjson::Value pwm_array(rapidjson::kArrayType);

            for (int ch = 0; ch < 8; ch++) {
                pwm_array.PushBack(pwm[ch] * 100, d.GetAllocator());
            }

            a3_value.AddMember("PWM", pwm_array, d.GetAllocator());
            d.AddMember("sim_status", a3_value, d.GetAllocator());

        }

        void simulation_dji_a3_adapter::on_simulator_link_failed(client *c, websocketpp::connection_hdl hdl) {
            std::cerr << "A3 simulator failed, will try later" << std::endl;
            this->sim_online = false;
        }

        void simulation_dji_a3_adapter::on_simulator_link_closed(client *c, websocketpp::connection_hdl hdl) {
            std::cerr << "A3 simulator disconnected, will try later" << std::endl;
            this->sim_online = false;
        }

        void simulation_dji_a3_adapter::on_assitant_open(client *c, websocketpp::connection_hdl hdl) {
            std::cout << "Connect to A3 successfully" << std::endl;
            this->assiant_online = true;
        }

        void simulation_dji_a3_adapter::on_assitant_failed(client *c, websocketpp::connection_hdl hdl) {
            this->assiant_online = false;
            std::cerr << "A3 Assitant failed or disconnected, will try later" << std::endl;
        }

        void simulation_dji_a3_adapter::send_realtime_data() {
            if (!this->sim_online) {
                return;
            }
            rapidjson::Document d;
            d.SetObject();
            d.AddMember("SEQ", "FUCK", d.GetAllocator());
            d.AddMember("CMD", "set_fixed_wing", d.GetAllocator());

            Eigen::Vector3d global_location = get_lati_lon_alti();
            d.AddMember("tick", (int64_t) simulator_tick, d.GetAllocator());
            d.AddMember("lati", global_location.x(), d.GetAllocator());
            d.AddMember("lon", global_location.y(), d.GetAllocator());
            d.AddMember("height", -global_location.z(), d.GetAllocator());
            d.AddMember("rho", 1.29f, d.GetAllocator());

            add_value(d,get_airspeed() , d, "airspeed");

            add_vector(d, get_acc_body_NED(), d, "acc");

            add_vector(d, get_ground_velocity_NED(), d, "vel");

            add_vector(d, get_angular_velocity_body_NED(), d, "angular_vel");

            add_attitude(d, get_quaternion_NED(), d, "q");


            int32_t size;
            const char *str = json_to_buffer(d, size);
            c_simulator->send(sim_connection_hdl, str, size, websocketpp::frame::opcode::text);

        }

        void simulation_dji_a3_adapter::try_connect_assistant() {
            if (!assiant_online) {
                websocketpp::lib::error_code ec;
                client::connection_ptr con = c_root.get_connection(root_uri, ec);
                if (ec) {
                    std::cerr << "could not create connection to A3 because: " << ec.message() << std::endl;
                    return;
                }
                std::cout << "Try to reconnect A3 assiant on " << root_uri << std::endl;
                c_root.connect(con);
            }

        }

        bool simulation_dji_a3_adapter::enable_simulation() {
            return (this->motor_starter && this->assiant_online);
        }

    }
}
