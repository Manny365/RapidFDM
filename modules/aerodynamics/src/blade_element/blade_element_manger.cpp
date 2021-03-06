//
// Created by Hao Xu on 16/7/18.
//


#include <RapidFDM/aerodynamics/blade_element/blade_element_manager.h>

namespace RapidFDM
{
    namespace Aerodynamics
    {
        void BladeElementManager::calculate_washes(AirState airState, double deltatime)
        {
            //TODO:
            for (auto pair : blade_elements) {
                pair.second->rho = airState.rho;
                pair.second->ground_air_speed = airState.ground_air_speed;
            }
        }

        void BladeElementManager::add_blade_element(BaseBladeElement *blade)
        {
            blade_elements[blade] = new AirState;
            blade_elements[blade]->ground_air_speed;
        }

        AirState BladeElementManager::get_blade_element_airstate(BaseBladeElement *element)
        {
            return *blade_elements[element];
        }

        void BladeElementManager::add_blade_elements(std::vector<BaseBladeElement *> blades)
        {
            for (auto blade : blades) {
                add_blade_element(blade);
            }
        }

        rapidjson::Document *BladeElementManager::get_blades_information()
        {
            rapidjson::Document *d = new rapidjson::Document;
            d->SetArray();

            for (auto pair:blade_elements) {
                AirState airState = *pair.second;
                BaseBladeElement * blade =pair.first;
                ComponentData data = pair.first->get_seted_component_data();
                Eigen::Matrix3d convert_coord = blade->get_body_transform().linear();
                Eigen::Vector3d force_body = convert_coord *blade->get_aerodynamics_force(data,airState);
                rapidjson::Value object(rapidjson::kObjectType);
                add_vector(object, force_body, *d, "force");
                Eigen::Vector3d location = (Eigen::Vector3d)blade->get_body_transform().translation();
                add_vector(object,location,*d,"location");
                d->PushBack(object,d->GetAllocator());
            }
            return d;

        }
        void BladeElementManager::calculate_forces_and_torques()
        {
            forces = Eigen::Vector3d(0,0,0);
            torques = Eigen::Vector3d(0,0,0);
            for (auto pair : blade_elements) {
                AirState airState = *pair.second;
                BaseBladeElement * blade =pair.first;
                ComponentData data = pair.first->get_seted_component_data();
                Eigen::Matrix3d convert_coord = blade->get_body_transform().linear();
                Eigen::Vector3d force_body = convert_coord *blade->get_aerodynamics_force(data,airState);
                forces += force_body;
                
                Eigen::Vector3d node_body_r = (Eigen::Vector3d) blade->get_body_transform().translation();
                torques += convert_coord * blade->get_aerodynamics_torque(data,airState) + node_body_r.cross(force_body);
            }
        }
        
        Eigen::Vector3d BladeElementManager::get_total_force()
        {
			return forces;
        }
        Eigen::Vector3d BladeElementManager::get_total_torque()
        {
            return torques;
        }
    }
    
}