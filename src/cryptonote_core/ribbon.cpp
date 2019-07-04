#include <vector>
#include <curl/curl.h>
#include <iostream>

#include "rapidjson/document.h"
#include "ribbon.h"

namespace service_nodes {

size_t curl_write_callback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// TODO: use http_client from net tools
std::vector<trade_oger_trade> get_trades_from_ogre()
{
  std::string url = "https://tradeogre.com/api/v1/history/BTC-XTRI";
  std::string read_buffer;
  CURL* curl = curl_easy_init(); 
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); 
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
  CURLcode res = curl_easy_perform(curl); 
  curl_easy_cleanup(curl); 
  if (res != CURLE_OK)
    std::cout << "not ok" << std::endl;
    
  rapidjson::Document document;
  document.Parse(read_buffer.c_str());
  
  std::vector<trade_oger_trade> trades;
  for (size_t i = 0; i < document.Size(); i++)
  {
    trade_oger_trade trade;
    trade.date = document[i]["date"].GetInt();
    trade.type = document[i]["type"].GetString();
    trade.type = document[i]["price"].GetString();
    trade.type = document[i]["quantity"].GetString(); 
    trades.push_back(trade);
  }
  
  return trades;
}

}
