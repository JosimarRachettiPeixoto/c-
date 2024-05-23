#include "crow_all.h"
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sw/redis++/redis++.h>
#include "spdlog/spdlog.h"

using namespace std;
using namespace sw::redis;
using json = nlohmann::json;

class File {
    private:
        std::string file;
    public:
        File(std::string file){this->file=file;};

        std::string write(json data){
            std::ofstream arquivo;
            arquivo.open(file, std::ofstream::app);
            arquivo << data << std::endl;
            arquivo.close();

            return "successfull write";
        };

        std::string read(){
            std::ifstream read_file(file);
            std::string str; 
            std::string response;
            while (std::getline(read_file, str))
            {
               response += str;
            }
        return response;
        };

        void delete_file(){
            std::filesystem::remove(file);
        }

        void create_file(){
            ofstream MyFile(file);
        }

        void update_file(json data){
            delete_file();
            create_file();
            write(data);            
        }

};

class InterfaceRedis{
    private:
        
    public:
        sw::redis::Redis redis = Redis("tcp://127.0.0.1");
        InterfaceRedis(){ };

        void set_value(std::string key, std::string value){
            redis.set(key, value);
        }

        std::string get_value(std::string key){
            Optional<std::string> res = redis.get(key);
            if (res){
                spdlog::info("find the key");
                return *res;
            }else{
                spdlog::warn("dont find the key");
                return "404";
            }
        }

        void delete_key(std::string key){
            long long res = redis.del(key);
        };
};

class HashKey {

    public:
        hash <string> hasher;
        std::string create(std::string user, std::string password){
            std::string key;
            key += user; 
            key += password;
            std::string hash_key;
            hash_key = hasher(key);
            return hash_key;
        }

};

int main()
{
    crow::SimpleApp app;
    using json = nlohmann::json;
   
    CROW_ROUTE(app, "/createnote/user/<string>/password/<string>")
    .methods("POST"_method)
    ([](const crow::request& req, string user, string password){
       
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        spdlog::info("enter create note for user " + user);    
        std::string hash_key = hasher_key.create(user, password);
    
        spdlog::info("validate user to create note");
        std::string res_redis = connect_redis.get_value(hash_key);
        if (res_redis != "ok"){
                return crow::response(400,"user not found.");
        };

        if(req.method == "POST"_method){

            spdlog::info("validate create body request to create note");
            json response_json = nlohmann::json::parse(req.body);
            if(!(response_json.contains(std::string{ "note" }))){
                return crow::response(400,"body not have field found.");
            }

            spdlog::info("create note");
            std::string user_file = hash_key;
            user_file += ".txt";

            File file_notes(user_file);

            std::string notes = file_notes.read();
            nlohmann::json notes_obj = nlohmann::json::parse(notes);

            spdlog::info("verificando se ja existe note para user" + user);
            if(notes_obj.contains(std::string{ "notes" })){
                spdlog::info("notas ja existem, adicionando notas");
                notes_obj["notes"].push_back(response_json["note"]);
            }else{
                spdlog::info("notas não existem criando notas e adicionando nota");
                std::vector<std::string> note;
                note.push_back(response_json["note"]);
                notes_obj["notes"]= note;
            }    
           
            file_notes.update_file(notes_obj);
            return crow::response(200,"Sua mensagem foi salva com sucesso.");
        };
        return crow::response(204,"method not allowed");
        
    });

    CROW_ROUTE(app, "/updatenote/user/<string>/password/<string>/index/<int>")
    .methods("PUT"_method)
    ([](const crow::request& req, string user, string password, int index){
       
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        spdlog::info("enter create note for user " + user);    
        std::string hash_key = hasher_key.create(user, password);
    
        spdlog::info("validate user to create note");
        std::string res_redis = connect_redis.get_value(hash_key);
        if (res_redis != "ok"){
                return crow::response(400,"user not found.");
        };

        if(req.method == "PUT"_method){
            json body_req = nlohmann::json::parse(req.body);
            
            if(!(body_req.contains(std::string{ "new_note" }))){
                spdlog::error("new_note not send in the body");
                return crow::response(400,"new_note is obligated fields.");
            }
            
            spdlog::info("validate user to update note", index);
            std::string user_file = hash_key;
            user_file += ".txt";

            File file_user_notes(user_file);
            std::string user_notes = file_user_notes.read();
            nlohmann::json notes = nlohmann::json::parse(user_notes);

            if(notes["notes"].size() < index){
                return crow::response(400, "index is not found in array notes.");
            }

            notes["notes"][index] = body_req["new_note"];
            file_user_notes.update_file(notes);
            return crow::response(200, notes["notes"][index]);
        };
       
        return crow::response(204,"method not allowed");
        
    });

    CROW_ROUTE(app, "/getnotes/user/<string>/password/<string>")
    .methods("GET"_method)
    ([](const crow::request& req, string user, string password){
       
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        spdlog::info("enter create note for user " + user);    
        std::string hash_key = hasher_key.create(user, password);
    
        spdlog::info("validate user to create note");
        std::string res_redis = connect_redis.get_value(hash_key);
        if (res_redis != "ok"){
                return crow::response(400,"user not found.");
        };

        if (req.method == "GET"_method){

            spdlog::info("get notes user");

            std::string user_file = hash_key;
            user_file += ".txt";

            File file_user_notes(user_file);
            std::string notes = file_user_notes.read();

            
            spdlog::info("parse notes to json");
            nlohmann::json notes_json = nlohmann::json::parse(notes);
            if(!notes_json.contains(std::string{ "notes" })){
                return crow::response(200, "user dont have notes.");
            }

            std::string response_notes;
            std::cout << notes_json << std::endl;
            int size_notes = notes_json["notes"].size() - 1;
            for(int i=0; i <= size_notes; i++){
                std::string note = notes_json["notes"][i];
                response_notes += "\n" + std::to_string(i) + ": " + note;
            }
            return crow::response(200, response_notes);
        };
        return crow::response(204,"method not allowed");
    });

    CROW_ROUTE(app, "/deletenote/user/<string>/password/<string>/index/<int>")
    .methods("DELETE"_method)
    ([](const crow::request& req, string user, string password, int index){
       
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        spdlog::info("enter delete note for user " + user);    
        std::string hash_key = hasher_key.create(user, password);
    
        spdlog::info("validate user to delete note");
        std::string res_redis = connect_redis.get_value(hash_key);
        if (res_redis != "ok"){
                return crow::response(400,"user not found.");
        };

        if(req.method == "DELETE"_method){
            
            std::vector<std::string> new_note;

            spdlog::info("validate user to delete note " + std::to_string(index));
            std::string user_file = hash_key;
            user_file += ".txt";

            spdlog::info("get file user notes");
            File file_user_notes(user_file);
            std::string user_notes = file_user_notes.read();
            nlohmann::json notes = nlohmann::json::parse(user_notes);

            spdlog::info("for notes user file", user_notes);
            int size_notes = notes["notes"].size();
          
            if(size_notes < index){
                return crow::response(400, "index is not found in array notes.");
            }

            spdlog::info(size_notes);
            int size_array = size_notes - 1;
            for(int i=0; i <= size_array; i++){
                if (i == index){
                    continue;
                };
                std::cout << notes["notes"][i] << std::endl;
                new_note.push_back(notes["notes"][i]);
            };
            notes["notes"] = new_note;
            spdlog::info("att file user notes");
            file_user_notes.update_file(notes);
            return crow::response(200, "ok");
        };

        return crow::response(204,"method not allowed");
        
    });

    CROW_ROUTE(app, "/createuser")
    .methods("GET"_method, "POST"_method, "PUT"_method, "DELETE"_method)
    ([](const crow::request& req){

        hash <string> hasher;
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        if(req.method == "POST"_method){
            json response_json = nlohmann::json::parse(req.body);
            spdlog::info("enter create user");
            if( !(response_json.contains(std::string{ "user" }))|| 
                !(response_json.contains(std::string{ "password" }))
            ){
                spdlog::error("user or password not send in the body");
                return crow::response(400,"user and password is obligated fields.");
            }
            
            spdlog::info("verify if user in redis");
            const std::string res_redis = connect_redis.get_value(response_json["user"]); 
            if(res_redis == "ok"){
                spdlog::error("user already exist");
                return crow::response(400,"user already exist");
            }

            spdlog::info("save user and password log file.");
            File file_user("usuarios.txt");
            file_user.write(response_json);

            spdlog::info("create hash user in db para login");
            std:string hash_key;
            hash_key = hasher_key.create(response_json["user"],  response_json["password"]);

            connect_redis.set_value(hash_key, "ok");

            std::string user_file = hash_key;
            user_file += ".txt";

            File file_notes(user_file);
            file_notes.create_file();
            file_notes.write({});
            return crow::response(200,"user create successfully.");
        };

        return crow::response(204,"method not allowed");
        
    });

    CROW_ROUTE(app, "/deleteuser/user/<string>/password/<string>")
    .methods("DELETE"_method)
    ([](const crow::request& req, string user, string password){

        hash <string> hasher;
        InterfaceRedis connect_redis;
        HashKey hasher_key;

        if(req.method == "DELETE"_method){
            
            spdlog::info("create hash user to verify in db");
            std:string hash_key;
            hash_key = hasher_key.create(user,  password);

            spdlog::info("verify if user in redis");
            const std::string res_redis = connect_redis.get_value(hash_key); 
            if(res_redis != "ok"){
                spdlog::error("user dont exist");
                return crow::response(400,"user dont exist");
            }

            std::string user_file = hash_key;
            user_file += ".txt";

            spdlog::info("delete hash user in db");
            File file_user(user_file);
            file_user.delete_file();

            spdlog::info("delete hash in redis");
            connect_redis.delete_key(hash_key);

            return crow::response(200,"user deleted successfully.");
        };

        return crow::response(204,"method not allowed");
        
    });

    app.port(3002).run();
    return 0;
}