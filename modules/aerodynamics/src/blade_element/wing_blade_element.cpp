//
// Created by Hao Xu on 16/7/18.
//

#include <RapidFDM/aerodynamics/blade_element/wing_blade_element.h>
#include <RapidFDM/aerodynamics/geometrys/Geometrys.h>
#include <RapidFDM/aerodynamics/nodes/wings/wing_node.h>


namespace RapidFDM
{
    namespace Aerodynamics
    {
        WingBladeElement::WingBladeElement(BaseGeometry *geo, double inner_span_percent, double outer_span_percent) :
                BaseBladeElement(geo)
        {
            assert(fabs(outer_span_percent) > fabs(inner_span_percent));
            assert(fabs(outer_span_percent) <= 1);
            if (fabs(1 - outer_span_percent) < 1e-3) {
                this->on_taper = true;
            }
            else {
                this->on_taper = false;
            }

            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geo);
            assert(wingGeometry != nullptr);

            this->deflectAngle = wingGeometry->params.deflectAngle;
            this->MidChordSweep = wingGeometry->params.MidChordSweep;

            mid_span_length =
                    (outer_span_percent + inner_span_percent) * wingGeometry->params.b_2 / 2 / cos(this->deflectAngle);
            double chord_center_position = 0.25 * wingGeometry->params.root_chord_length // root chord center offset part
                                          + tan(wingGeometry->params.MidChordSweep) *
                                            fabs(outer_span_percent + inner_span_percent) / 2 *
                                            wingGeometry->params.b_2; // sweep part
            Eigen::Vector3d relative_pos = Eigen::Vector3d(
                    chord_center_position,
                    mid_span_length,
                    fabs(outer_span_percent + inner_span_percent) / 2 * tan(this->deflectAngle) * wingGeometry->params.b_2
            );

            Eigen::Vector3d rotate_axis = Eigen::Vector3d(1, 0, 0);
            if (mid_span_length < 0) {
                rotate_axis = -rotate_axis;
            }

            Eigen::Quaterniond element_attitude(Eigen::AngleAxisd(deflectAngle, rotate_axis));
            relative_transform.fromPositionOrientationScale(relative_pos, element_attitude, Eigen::Vector3d(1, 1, 1));

            double span_ratio = fabs(outer_span_percent + inner_span_percent) / 2;
            this->Mac = (1 - span_ratio) * wingGeometry->params.root_chord_length +
                        span_ratio * wingGeometry->params.taper_chord_length;
//            printf("name %s span ratio %f root chord %f taper %f\n",
//                   this->geometry->getName().c_str(),
//                   span_ratio,
//                   wingGeometry->params.root_chord_length,
//                   wingGeometry->params.taper_chord_length
//            );

            this->element_span_length = fabs(outer_span_percent - inner_span_percent) * wingGeometry->params.b_2;
        }

        double WingBladeElement::getLift(ComponentData state, AirState airState) const
        {

            double cl = get_cl(state, airState);
            double lift = cl * state.get_q_bar(airState) * this->Mac * this->element_span_length;
            return lift + get_flap_lift(state,airState);
        }

        double WingBladeElement::getDrag(ComponentData state, AirState airState) const
        {
            double cd = get_cd(state, airState);
            return cd * state.get_q_bar(airState) * this->Mac * this->element_span_length + get_flap_drag(state,airState);
        }

        double WingBladeElement::getSide(ComponentData state, AirState airState) const
        {
            //TODO:
            //real side force
            return 0;
        }
        float WingBladeElement::get_flap_lift(ComponentData state, AirState airState) const {
            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geometry);
            double cl_by_control = 0;
            if (get_wing_node()->enableControl)
            {
                double internal = 0;
                if(wingGeometry->params.wingPart == 2) {
                    if (this->mid_span_length < 0) {
                        //LEFT SIDE
                        internal = get_wing_node()->get_internal_state("flap_0");
                    }
                    else
                    {
                        //Right Side
                        internal = get_wing_node()->get_internal_state("flap_1");
                    }
                }
                else {
                    internal = get_wing_node()->get_internal_state("flap");
                }
                cl_by_control = internal * wingGeometry->params.cl_by_deg * wingGeometry->params.maxdeflect;
            }

            double x = state.get_angle_of_attack(airState) * 180.0 / M_PI;
            if (x  > wingGeometry->params.stall_angle || x < - wingGeometry->params.stall_angle) {
                cl_by_control = 0;
            }
            double res =  cl_by_control * state.get_q_bar(airState) * this->Mac * this->element_span_length * wingGeometry->params.ctrlSurfFrac;
            return res;
        }
        float WingBladeElement::get_flap_drag(ComponentData state, AirState airState) const {
            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geometry);
            double cd_by_control = 0;
            if (get_wing_node()->enableControl)
            {
                double internal = 0;
                if(wingGeometry->params.wingPart == 2) {
                    if (this->mid_span_length < 0) {
                        //LEFT SIDE
                        internal = get_wing_node()->get_internal_state("flap_0");
                    }
                    else
                    {
                        //Right Side
                        internal = get_wing_node()->get_internal_state("flap_1");
                    }
                }
                else {
                    internal = get_wing_node()->get_internal_state("flap");
                }
                double alpha = internal * wingGeometry->params.maxdeflect;
                cd_by_control = wingGeometry->params.cd_by_deg2 * alpha * alpha;
            }

            double res =  cd_by_control * state.get_q_bar(airState) * this->Mac * this->element_span_length * wingGeometry->params.ctrlSurfFrac;
            return res;
        }


        float WingBladeElement::get_cl(ComponentData state, AirState airState) const
        {
            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geometry);
            double x = state.get_angle_of_attack(airState) * 180.0 / M_PI;
            double cl = wingGeometry->params.cl0 + wingGeometry->params.cl_by_deg * x;
            //Stall
            if (x  > wingGeometry->params.stall_angle || x < - wingGeometry->params.stall_angle) {
                cl = 0;
            }
            return float_constrain(cl,-2,2);
        }

        float WingBladeElement::get_cd(ComponentData state, AirState airState) const
        {
            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geometry);
            double alpha = state.get_angle_of_attack(airState) * 180 / M_PI;
            double cl = get_cl(state, airState);
            double cd = wingGeometry->params.cd0 + wingGeometry->params.cd_by_deg2 * alpha * alpha;
            cd = cd + cl * cl / M_PI / (2 * wingGeometry->params.b_2 / wingGeometry->params.Mac);
            return float_constrain(cd,-1.2,1.2);
        }

        float WingBladeElement::get_cm(ComponentData state, AirState airState) const
        {
            return 0;
        }

        Eigen::Vector3d WingBladeElement::get_aerodynamics_torque(ComponentData data, AirState airState) const
        {
            WingGeometry *wingGeometry = dynamic_cast<WingGeometry * >(geometry);
            double torque_by_flap = this->Mac * (0.75 -  wingGeometry->params.ctrlSurfFrac) * get_flap_lift(data,airState);
            return Eigen::Vector3d(0, -torque_by_flap, 0);
        }

        Eigen::Affine3d WingBladeElement::get_relative_transform() const
        {
            return relative_transform;
        }

        WingNode* WingBladeElement::get_wing_node() const
        {
            return dynamic_cast<WingNode*>(geometry->_parent);
        }
    }
}