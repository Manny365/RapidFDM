#ifndef RAPIDFDM_STANDARD_GEOMETRY_H
#define RAPIDFDM_STANDARD_GEOMETRY_H

#include <RapidFDM/aerodynamics/geometrys/base_geometry.h>

namespace RapidFDM
{
    namespace Aerodynamics
    {
        //! https://en.wikipedia.org/wiki/Drag_coefficient
        //  Result will be wrong when the box is very thin
        // Use Cd = 1
        class BoxGeometry : public BaseGeometry
        {
        protected:
            Eigen::Vector3d box_scale;
            Eigen::Vector3d reference_aera;
        public:
            BoxGeometry(const rapidjson::Value &v) :
                    BaseGeometry(v)
            {
                box_scale = fast_vector3(v, "scale");
                this->aera = fast_value(v,"aera");
                reference_aera.x() = box_scale.y() * box_scale.z();
                reference_aera.y() = box_scale.x() * box_scale.z();
                reference_aera.z() = box_scale.x() * box_scale.y();
            }

            virtual double getLift(ComponentData state, AirState airState) const override
            {
                double airspeed = state.get_relative_airspeed(airState).z();
                return  - airspeed*fabs(airspeed)*0.5*1.29f * aera * 1.06f;
            }

            virtual double getDrag(ComponentData state, AirState airState) const override
            {
                double airspeed = state.get_relative_airspeed(airState).x();
                return - airspeed*fabs(airspeed)*0.5*1.29f * aera * 1.06f;
            }

            virtual double getSide(ComponentData state, AirState airState) const override
            {
                double airspeed = state.get_relative_airspeed(airState).y();
                return - airspeed*fabs(airspeed)*0.5*1.29f * aera * 1.06f;
            }

            virtual void brief() override
            {
                printf("Box geometry \n");
                printf("Scale %f %f %f\n", box_scale.x(), box_scale.y(), box_scale.z());
            }
        };
    }
}


#endif
