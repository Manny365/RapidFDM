//
// Created by Hao Xu on 16/7/18.
//

#include <RapidFDM/aerodynamics/blade_element/wing_blade_element.h>
#include <RapidFDM/aerodynamics/geometrys/Geometrys.h>


namespace RapidFDM{
    namespace Aerodynamics{
        WingBladeElement::WingBladeElement(BaseGeometry *geo,double inner_span_percent,double outer_span_percent):
                BaseBladeElement(geo)
        {
            assert(abs(outer_span_percent) > abs(inner_span_percent));
            assert(abs(outer_span_percent) <= 1);
            if (abs(1-outer_span_percent)<1e-3)
            {
                this->on_taper = true;
            }
            else{
                this->on_taper = false;
            }

            WingGeometry * wingGeometry = dynamic_cast<WingGeometry * >(geo);
            assert(wingGeometry != nullptr);

            this->deflectAngle = wingGeometry->params.deflectAngle;
            this->MidChordSweep = wingGeometry->params.MidChordSweep;

            float mid_span_length = (outer_span_percent + inner_span_percent) *wingGeometry->params.b_2/ 2 /cos(this->deflectAngle) ;
            float chord_center_position = 0.25 * wingGeometry->params.root_chord_length // root chord center offset part
                                          + tan(wingGeometry->params.MidChordSweep) * abs(outer_span_percent + inner_span_percent)  / 2 * wingGeometry->params.b_2; // sweep part
            Eigen::Vector3d relative_pos = Eigen::Vector3d(
                   chord_center_position,
                   mid_span_length,
                   (outer_span_percent + inner_span_percent) / 2 * tan(this->deflectAngle) * wingGeometry->params.b_2
            );

            Eigen::Vector3d rotate_axis  =Eigen::Vector3d(1,0,0);
            if (mid_span_length < 0)
            {
                rotate_axis = - rotate_axis;
            }

            Eigen::Quaterniond element_attitude(Eigen::AngleAxisd(deflectAngle,rotate_axis));
            relative_transform.fromPositionOrientationScale(relative_pos,element_attitude,Eigen::Vector3d(1,1,1));

            float span_ratio = abs(outer_span_percent + inner_span_percent ) / 2 / wingGeometry->params.b_2;
            this->Mac = (1-span_ratio) * wingGeometry->params.root_chord_length + span_ratio * wingGeometry->params.taper_chord_length;
            this->element_span_length = abs(outer_span_percent - inner_span_percent) * wingGeometry->params.b_2;
        }
        float WingBladeElement::getLift(ComponentData state, AirState airState) const
        {
            double velocity = state.get_airspeed_mag(airState);
            double cl = get_cl(state,airState);
            return cl * state.get_q_bar(airState) * this->Mac * this->element_span_length;
        }
        float WingBladeElement::getDrag(ComponentData state, AirState airState) const
        {
            double velocity = state.get_airspeed_mag(airState);
            double cd = get_cd(state,airState);
            return cd * state.get_q_bar(airState) * this->Mac * this->element_span_length;
        }
        float WingBladeElement::getSide(ComponentData state, AirState airState) const
        {
            //TODO:
            //real side force
            return 0;
        }

        float WingBladeElement::get_cl(ComponentData state, AirState airState) const
        {
            double velocity = state.get_airspeed_mag(airState);
            double x = state.get_angle_of_attack(airState);
            double cl = 0.25 + 5.27966 * x + 0.812763 * x * x - 5.66835  * x * x * x - 13.7039 * x * x * x  *x;
//            double cl = 2 * M_PI * x;
            cl = cl /4.302;
            //Stall
            if (x * 180 / M_PI > 15 || x * 180 / M_PI < -15)
                cl = 0;
            return cl;
        }

        float WingBladeElement::get_cd(ComponentData state, AirState airState) const
        {
            double alpha = state.get_angle_of_attack(airState);
            double cl = get_cl(state,airState);
            double cd = 0.0109392 + 0.494631 * alpha * alpha;
            cd = cd /4.302 + cl * cl;
            return cd;
        }
    }
}