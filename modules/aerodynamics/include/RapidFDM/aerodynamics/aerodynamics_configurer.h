//
// Created by Hao Xu on 16/6/11.
//

#ifndef RAPIDFDM_AERODYNAMICS_CONFIGURER_H
#define RAPIDFDM_AERODYNAMICS_CONFIGURER_H

#include "flying_data_defines.h"
#include "aerodynamics.h"
#include <vector>
#include <map>
#include <functional>
#include <string>
#include <rapidjson/document.h>
#include "airdynamics_parser.h"
#include <RapidFDM/utils.h>
/*
 * configure_server
 * chroot folder
 * load a model
 * save a model
 * update model from json defines
 * transfer model datas
 */

namespace RapidFDM
{
    namespace Aerodynamics
    {
        class aerodynamics_configurer
        {
        protected:
            std::string root_path;
            parser *parser1 = nullptr;
            typedef std::function<rapidjson::Document *(const rapidjson::Value &)> query_function;
            std::map<std::string, query_function> query_functions;
            std::map<std::string, query_function> query_configure_functions;
            AircraftNode *aircraftNode;
        public:
            aerodynamics_configurer(std::string root_path);

            //! Chroot to a folder
            std::vector<std::string> chroot_folder(std::string path);

            //!List model
            std::vector<std::string> list_model();

            //!Load model from root
            const rapidjson::Value & load_model(std::string name);

            //!Save model to root
            void save_model(std::string name);

            //!update model from json defs
            void update_model(const rapidjson::Value &v);

            //!Query information of this model
            rapidjson::Document *query_model(const rapidjson::Value &v);

            //!Operation of this configurer
            rapidjson::Document *query_configurer(const rapidjson::Value &v);


            void init_query_functions();
        };

    }
}

#endif //RAPIDFDM_AERODYNAMICS_CONFIGURER_H
