//
// Created by Hao Xu on 16/7/18.
//

#ifndef RAPIDFDM_WING_BLADE_ELEMENT_H
#define RAPIDFDM_WING_BLADE_ELEMENT_H

#include <RapidFDM/aerodynamics/blade_element/base_blade_element.h>
namespace RapidFDM
{
    namespace Aerodynamics
    {
        class WingBladeElement : public BaseBladeElement {
        protected:
            //This transform shall be relative to
            Eigen::Affine3d relative_transform;
            float Mac = 0;
            bool on_taper = false;
            float deflectAngle;
            float MidChordSweep;
            float element_span_length;
        public:
            WingBladeElement(BaseGeometry * geo,double inner_span_percent,double outer_span_percent);

            virtual float get_cl(ComponentData state,AirState airState) const;

            virtual float get_cd(ComponentData state,AirState airState) const;

//            virtual float get_cs(ComponentData state,AirState airState) const;

            virtual float getLift(ComponentData state, AirState airState) const override;

            virtual float getDrag(ComponentData state, AirState airState) const override;

            virtual float getSide(ComponentData state, AirState airState) const override;


        };
    }
}
#endif //RAPIDFDM_WING_BLADE_ELEMENT_H
