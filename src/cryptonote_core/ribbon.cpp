#include <vector>
#include <curl/curl.h>
#include <iostream>

#include "rapidjson/document.h"
#include "blockchain.h"
#include "ribbon.h"

namespace service_nodes {

cryptonote::Blockchain* m_blockchain_storage;

size_t curl_write_callback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// TODO: use http_client from net tools
std::string make_curl_http_get(std::string url)
{
  std::string read_buffer;
  CURL* curl = curl_easy_init(); 
  
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str()); 
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &read_buffer);
  curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
  CURLcode res = curl_easy_perform(curl); 
  curl_easy_cleanup(curl);
  
  return read_buffer;
}

std::vector<exchange_trade> get_trades_from_ogre()
{
  std::string data = make_curl_http_get(std::string(TRADE_OGRE_API) + std::string("/history/BTC-XTRI"));
    
  rapidjson::Document document;
  document.Parse(data.c_str());
  
  std::vector<exchange_trade> trades;
  for (size_t i = 0; i < document.Size() + 1; i++)
  {
    exchange_trade trade;
    trade.date = document[i]["date"].GetUint64();
    trade.type = document[i]["type"].GetString();
    trade.price = std::stod(document[i]["price"].GetString()); // trade ogre gives this info as a string
    trade.quantity = std::stod(document[i]["quantity"].GetString());
    trades.push_back(trade);
  }
  
  return trades;
}

std::vector<exchange_trade> get_trades_from_bitliber()
{
  std::string data = make_curl_http_get(std::string(BITLIBER_API) + std::string("/public/history/XTRI-BTC"));
    
  rapidjson::Document document;
  document.Parse(data.c_str());
  
  std::vector<exchange_trade> trades;
  std::cout << "doc size: " << document.Size() + 1 << std::endl;
  for (size_t i = 0; i < document.Size() + 1; i++)
  {
    exchange_trade trade;
    trade.date = std::stoull(document["result"][i]["date"].GetString()); // bitliber gives this info as a string
    trade.type = document["result"][i]["type"].GetString();
    trade.price = document["result"][i]["price"].GetDouble();
    trade.quantity = document["result"][i]["quantity"].GetDouble(); 
    trades.push_back(trade);
  }
  
  return trades;
}

std::vector<exchange_trade> trades_during_latest_20_blocks(std::vector<exchange_trade> trades)
{
  uint64_t top_block_height = m_blockchain_storage->get_current_blockchain_height() - 1;
  crypto::hash top_block_hash = m_blockchain_storage->get_block_id_by_height(top_block_height);
  cryptonote::block top_blk;
  m_blockchain_storage->get_block_by_hash(top_block_hash, top_blk);
  uint64_t top_block_timestamp = top_blk.timestamp;
  
  uint64_t twenty_block_height = m_blockchain_storage->get_current_blockchain_height() - 21;
  crypto::hash twenty_block_hash = m_blockchain_storage->get_block_id_by_height(twenty_block_height);
  cryptonote::block twenty_blk;
  m_blockchain_storage->get_block_by_hash(twenty_block_hash, twenty_blk);
  uint64_t twenty_block_timestamp = twenty_blk.timestamp;
  
  std::vector<exchange_trade> result;
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].date <= top_block_timestamp && trades[i].date >= twenty_block_timestamp)
      result.push_back(trades[i]);
  }
  
  return result;
}

double trades_weighted_mean(std::vector<exchange_trade> trades)
{
  double XTRI_volume_sum = 0;
  double weighted_sum = 0;
  for (size_t i = 0; i < trades.size(); i++)
  {
    XTRI_volume_sum += trades[i].quantity;
    weighted_sum += (trades[i].price * trades[i].quantity);
  }
  
  return weighted_sum / XTRI_volume_sum;
}

}
