//
// Created by Hao Xu on 16/8/11.
//

#include "RapidFDM/simulation/simulation_dji_a3_adapter.h"

#define C_EARTH 6378137.0f

#define MIXER_GENERAL

namespace RapidFDM
{
    namespace Simulation
    {
        simulation_dji_a3_adapter::simulation_dji_a3_adapter(AircraftNode *aircraft) :
                interval(10)
        {
            this->aircraft = aircraft;

//            root_uri = "ws://10.60.23.132:19870/general/";
//            sim_uri = "ws://10.60.23.132:19870/controller/simulator/";
            root_uri = "ws://10.211.55.3:19870/general";
            sim_uri = "ws://10.211.55.3:19870/controller/simulator/";
            
            c_root.set_access_channels(websocketpp::log::alevel::none);
            c_root.clear_access_channels(websocketpp::log::alevel::all);
            
            c_root.init_asio();
            
            c_root.set_message_handler(bind(&simulation_dji_a3_adapter::on_message_root, this, &c_root, ::_1, ::_2));
            c_root.set_fail_handler(bind(&simulation_dji_a3_adapter::on_assitant_failed, this, &c_root, ::_1));
            c_root.set_close_handler(bind(&simulation_dji_a3_adapter::on_assitant_failed, this, &c_root, ::_1));
            
            c_root.set_open_handler(bind(&simulation_dji_a3_adapter::on_assitant_open, this, &c_root, ::_1));
            
            websocketpp::lib::error_code ec;
            client::connection_ptr con = c_root.get_connection(root_uri, ec);
            if (ec) {
                std::cout << "could not create connection to A3 because: " << ec.message() << std::endl;
                std::abort();
            }
            
            c_root.connect(con);
            
            
            timer = new boost::asio::deadline_timer(c_root.get_io_service(), interval);
            
            timer->async_wait([&](const boost::system::error_code &) {
                this->tick();
            });
            
        }
        
        void simulation_dji_a3_adapter::on_message_root(client *c, websocketpp::connection_hdl hdl, message_ptr msg)
        {
            
            rapidjson::Document d;
            d.Parse(msg->get_payload().c_str());
            if (!d.IsObject() || !d.HasMember("FILE") || !d["FILE"].IsString()) {
                std::wcerr << "Parse a3 assiant document failed!" << std::endl;
                return;
            }
            
            file_name = d["FILE"].GetString();
            std::cout << "Try to connect simulator on " << file_name << std::endl;
            
            std::string uri = sim_uri + file_name;
            websocketpp::lib::error_code ec;
            std::cout << uri << std::endl;
            c_simulator = new client;
            c_simulator->set_access_channels(websocketpp::log::alevel::none);
            c_simulator->clear_access_channels(websocketpp::log::alevel::all);
            c_simulator->init_asio(&c_root.get_io_service());
            
            
            c_simulator->set_message_handler(
                    bind(&simulation_dji_a3_adapter::on_message_simulator, this, c_simulator, ::_1, ::_2));
            c_simulator->set_open_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_open, this, c_simulator, ::_1));
            
            c_simulator->set_close_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_open, this, c_simulator, ::_1));
            
            c_simulator->set_fail_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_failed, this, c_simulator, ::_1));
            
            client::connection_ptr con = c_simulator->get_connection(uri, ec);
            
            c_simulator->connect(con);
            
            
        }
        
        void simulation_dji_a3_adapter::reconnect_simulator()
        {
            std::cout << "Try to connect simulator on " << file_name << std::endl;
            
            std::string uri = sim_uri + file_name;
            websocketpp::lib::error_code ec;
            std::cout << uri << std::endl;
            c_simulator = new client;
            c_simulator->set_access_channels(websocketpp::log::alevel::none);
            c_simulator->clear_access_channels(websocketpp::log::alevel::all);
            c_simulator->init_asio(&c_root.get_io_service());
            
            
            c_simulator->set_message_handler(
                    bind(&simulation_dji_a3_adapter::on_message_simulator, this, c_simulator, ::_1, ::_2));
            c_simulator->set_open_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_open, this, c_simulator, ::_1));
            
            c_simulator->set_close_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_open, this, c_simulator, ::_1));
            
            c_simulator->set_fail_handler(
                    bind(&simulation_dji_a3_adapter::on_simulator_link_failed, this, c_simulator, ::_1));
            
            client::connection_ptr con = c_simulator->get_connection(uri, ec);
            
            c_simulator->connect(con);
            
        }
        
        void simulation_dji_a3_adapter::on_simulator_link_open(client *c, websocketpp::connection_hdl hdl)
        {
            rapidjson::Document d;
            d.SetObject();
            d.AddMember("SEQ", "FUCK", d.GetAllocator());
            d.AddMember("CMD", "start_sim", d.GetAllocator());
            d.AddMember("latitude", 0, d.GetAllocator());
            d.AddMember("longitude", 0, d.GetAllocator());
            d.AddMember("frequency", 100, d.GetAllocator());
            d.AddMember("only_aircraft", 1, d.GetAllocator());
            sim_connection_hdl = hdl;
            int32_t size;
            const char *str = json_to_buffer(d, size);
            c->send(hdl, str, size, websocketpp::frame::opcode::text);
            sim_hdl = hdl;
            
            
        }
        
        void simulation_dji_a3_adapter::tick()
        {
            
            timer->expires_at(timer->expires_at() + interval);
            
            check_assiant_online();
            
            if (sim_online) {
                static int fcount = 0;
                if (fcount++ % 30 == 0) {
                    printf("fcount %d d tick %d\n", fcount, dcount);
                    dcount = 0;
                }
                if (!data_update) {
                    no_data_count++;
                }
                else {
                    data_update = false;
                    no_data_count = 0;
                }
                
                if (no_data_count > 100) {
                    printf("Simulator off line: nodata, will reconnect later\n");
                    std::string reason = "reconnect";
                    c_simulator->close(sim_hdl, websocketpp::close::status::normal, reason);
                    sim_online = false;
                }
                else {
                    send_realtime_data();
                }
            }
            else {
                if (assiant_online) {
                    static int count = 0;
                    if (count++ % 100 == 0) {
                        printf("Reconnect simulator\n");
                        reconnect_simulator();
                    }
                }
            }
            
            
            timer->async_wait([&](const boost::system::error_code &) {
                this->tick();
            });
        }
        
        void simulation_dji_a3_adapter::on_message_simulator(client *c, websocketpp::connection_hdl hdl,
                                                             message_ptr msg)
        {
            /*
             *      on_message_si
                    "AccelerometerX": 0,
                    "AccelerometerY": 0,
                    "AccelerometerZ": -1,
                    "EVENT": "sim_state",
                    "FlyingState": 0,
                    "GimbalPitch": 0,
                    "GimbalRoll": 0,
                    "GimbalYaw": 0,
                    "GyroX": 0,
                    "GyroY": 0,
                    "GyroZ": 0,
                    "MotorStarted": 0,
                    "Pitch": 0,
                    "ProductType": 6,
                    "Quaternion0": 0,
                    "Quaternion1": 0,
                    "Quaternion2": 1.2246468525851679e-16,
                    "Quaternion3": 0,
                    "RcA": 0,
                    "RcE": 0,
                    "RcR": 0,
                    "RcT": 0,
                    "Roll": 0,
                    "SimulatorCommand": 3,
                    "Time": 0,
                    "TransformState": 0,
                    "VelocityX": 0,
                    "VelocityY": 0,
                    "VelocityZ": 0,
                    "WorldLatitude": 0,
                    "WorldLongitude": 0,
                    "WorldX": 0,
                    "WorldY": 0,
                    "WorldZ": 0.10000000149011612,
                    "Yaw": 0
        }
             */
            rapidjson::Document d;
            d.Parse(msg->get_payload().c_str());
            
            assert(d.IsObject());
            this->sim_online = true;
            if (fast_string(d, "EVENT") == "sim_state") {
                motor_starter = fast_value(d, "MotorStarted") > 0;
                RcA = fast_value(d, "RcA");
                RcE = fast_value(d, "RcE");
                RcR = fast_value(d, "RcR");
                RcT = fast_value(d, "RcT");
            }
            else if (fast_string(d, "EVENT") == "sim_pwm") {
                for (int chn = 0; chn < 8; chn++) {
                    pwm[chn] = d["channels"][chn].GetDouble();
                    pwm[chn] = (pwm[chn] / 10000 - 0.5)*2;
                }
#ifdef MIXER_GENERAL
                aircraft->set_control_value("main_engine_0/thrust", (pwm[0] + 1) * 0.5);
                aircraft->set_control_value("main_engine_1/thrust", (pwm[1] + 1) * 0.5);
                aircraft->set_control_value("main_wing_0/flap_0",  pwm[2]);
                aircraft->set_control_value("main_wing_0/flap_1", - pwm[5]);
                aircraft->set_control_value("horizon_wing_0/flap_0", pwm[3]);
                aircraft->set_control_value("horizon_wing_0/flap_1", pwm[3]);
                aircraft->set_control_value("vertical_wing_0/flap", pwm[4]);
#endif

#ifdef MIXER_FLYINGWING
                aircraft->set_control_value("main_engine_0/thrust", (pwm[0] + 1) * 0.5);
                aircraft->set_control_value("main_wing_0/flap_0",  pwm[2]);
                aircraft->set_control_value("main_wing_0/flap_1", - pwm[3]);
                //aircraft->set_control_value("horizon_wing_0/flap_0", pwm[3]);
                //aircraft->set_control_value("horizon_wing_0/flap_1", pwm[3]);
                aircraft->set_control_value("vertical_wing_0/flap", pwm[4]);
#endif

#ifdef MIXER_VTOL
                aircraft->set_control_value("main_engine_0/thrust", (pwm[0] + 1) * 0.5);
                aircraft->set_control_value("main_engine_1/thrust", (pwm[1] + 1) * 0.5);
                aircraft->set_control_value("main_engine_2/thrust", (pwm[2] + 1) * 0.5);
                aircraft->set_control_value("main_engine_3/thrust", (pwm[3] + 1) * 0.5);
                aircraft->set_control_value("main_wing_0/flap_0",  pwm[4]);
                aircraft->set_control_value("main_wing_0/flap_1", - pwm[5]);
                aircraft->set_control_value("vertical_wing_0/flap", pwm[6]);
#endif
            }
            
            data_update = true;
            
            dcount++;
        }
        
        void simulation_dji_a3_adapter::add_values(rapidjson::Document &d)
        {
            rapidjson::Value a3_value(rapidjson::kObjectType);
            add_value(a3_value, motor_starter, d, "motor_started");
            add_value(a3_value, RcA, d, "RcA");
            add_value(a3_value, RcE, d, "RcE");
            add_value(a3_value, RcR, d, "RcR");
            add_value(a3_value, RcT, d, "RcT");
            add_value(a3_value, (int) sim_online, d, "online");
            
            rapidjson::Value pwm_array(rapidjson::kArrayType);
            
            for (int ch = 0; ch < 8; ch++) {
                pwm_array.PushBack(pwm[ch] * 100, d.GetAllocator());
            }
            
            a3_value.AddMember("PWM", pwm_array, d.GetAllocator());
            d.AddMember("a3_sim_status", a3_value, d.GetAllocator());
        }
        
        void simulation_dji_a3_adapter::on_simulator_link_failed(client *c, websocketpp::connection_hdl hdl)
        {
            std::cerr << "A3 simulator failed or disconnected, will try later" << std::endl;
            this->sim_online = false;
        }
        
        void simulation_dji_a3_adapter::on_assitant_open(client *c, websocketpp::connection_hdl hdl)
        {
            std::cout << "Connect to A3 successfully" << std::endl;
            this->assiant_online = true;
        }
        
        void simulation_dji_a3_adapter::on_assitant_failed(client *c, websocketpp::connection_hdl hdl)
        {
            std::cerr << "A3 Assitant failed or disconnected, will try later" << std::endl;
            this->assiant_online = false;
        }
        
        void simulation_dji_a3_adapter::send_realtime_data()
        {
            rapidjson::Document d;
            d.SetObject();
            d.AddMember("SEQ", "FUCK", d.GetAllocator());
            d.AddMember("CMD", "set_fixed_wing", d.GetAllocator());
            Eigen::Quaterniond convert = (Eigen::Quaterniond) Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitY());
            //TODO:Fix convention
            
            Eigen::Vector3d pos = (Eigen::Vector3d) aircraft->get_ground_transform().translation();
            pos = convert * pos;
            
            d.AddMember("tick", (int64_t)simulator_tick, d.GetAllocator());
            d.AddMember("lati", intial_lati  + pos.x()/C_EARTH, d.GetAllocator());
            d.AddMember("lon",intial_lon + pos.y()/ C_EARTH  / cos(intial_lati + pos.x()/C_EARTH), d.GetAllocator());
            d.AddMember("height",-  pos.z(), d.GetAllocator());
            d.AddMember("rho", 1.29f, d.GetAllocator());
            
            AirState airState;
            ComponentData data = aircraft->get_component_data();
            add_value(d, data.get_airspeed_mag(airState), d, "airspeed");
            
            Eigen::Vector3d acc = sim_air->gAcc;
            acc = convert * acc;
            acc.z() = acc.z() + 9.81;
            add_vector(d, acc, d, "acc");
            
            Eigen::Vector3d vel = convert * aircraft->get_ground_velocity();
            add_vector(d, vel, d, "vel");
            
            Eigen::Vector3d w = convert * aircraft->get_angular_velocity();
            add_vector(d, w, d, "angular_vel");
            
            Eigen::Quaterniond quat =
                    convert * (Eigen::Quaterniond) aircraft->get_ground_transform().rotation() * convert;
            
            add_attitude(d, quat, d, "q");
            
            
            int32_t size;
            const char *str = json_to_buffer(d, size);
            c_simulator->send(sim_connection_hdl, str, size, websocketpp::frame::opcode::text);
            
        }
        
        void simulation_dji_a3_adapter::check_assiant_online()
        {
            static int count = 0;
            if (count++ % 50 == 0 && !assiant_online) {
                websocketpp::lib::error_code ec;
                client::connection_ptr con = c_root.get_connection(root_uri, ec);
                if (ec) {
                    std::cout << "could not create connection to A3 because: " << ec.message() << std::endl;
                    std::abort();
                }
                std::cout << "Try to reconnect A3 assiant on " << root_uri << std::endl;
                c_root.connect(con);
                
            }
            
        }
    }
}
