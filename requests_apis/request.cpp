#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

static size_t WriteCallback(char *contents, size_t size, size_t nmeb, char *buffer_in){
    ((std::string*)buffer_in)->append((char*)contents, size * nmeb);    
    return size * nmeb;

}

int main(){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    using json = nlohmann::json;
    curl = curl_easy_init();
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.adviceslip.com/advice");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        json response_json = nlohmann::json::parse(readBuffer);
        json teste = response_json["slip"];
        std::cout << teste << std::endl;
    }

}