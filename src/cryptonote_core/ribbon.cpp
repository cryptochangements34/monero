#include <vector>
#include <curl/curl.h>
#include <iostream>
#include <math.h>

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
  std::cout << "doc size: " << document.Size() << std::endl;
  std::vector<exchange_trade> trades;
  for (size_t i = 0; i < document.Size(); i++)
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

std::vector<exchange_trade> get_recent_trades()
{
  return get_trades_from_ogre();
  // TODO: add other exchanges
}

double get_coinbase_pro_btc_usd()
{
  std::string data = make_curl_http_get(std::string(COINBASE_PRO) + std::string("/products/BTC-USD/ticker"));
  rapidjson::Document document;
  document.Parse(data.c_str());

  std::cout << "doc size: " << document.Size() << std::endl;
  double btc_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["result"]["price"].GetString());
  }
  return btc_usd;
}

double get_gemini_btc_usd()
{
  std::string data = make_curl_http_get(std::string(GEMINI_API) + std::string("/trades/btcusd?limit_trades=1"));
  rapidjson::Document document;
  document.Parse(data.c_str());
  
  std::cout << "doc size: " << document.Size() << std::endl;
  double btc_usd = 0;
  for (size_t i = 0; i < document.Size(); i++)
  {
    btc_usd = std::stod(document["result"][0]["price"].GetString());
  }
  return btc_usd;
}

std::vector<exchange_trade> trades_during_latest_1_block()
{
  std::vector<exchange_trade> trades = get_recent_trades();
  uint64_t top_block_height = m_blockchain_storage->get_current_blockchain_height() - 2;
  crypto::hash top_block_hash = m_blockchain_storage->get_block_id_by_height(top_block_height);
  cryptonote::block top_blk;
  m_blockchain_storage->get_block_by_hash(top_block_hash, top_blk);
  uint64_t top_block_timestamp = top_blk.timestamp;

  std::vector<exchange_trade> result;
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].date >= top_block_timestamp){
      result.push_back(trades[i]);
    }
  }
  return result;
}

std::vector<exchange_trade> filter_trades_during_block(std::vector<exchange_trade> trades, uint64_t block_height)
{
  cryptonote::block blk, prev_blk;
  crypto::hash block_hash = m_blockchain_storage->get_block_id_by_height(block_height);
  m_blockchain_storage->get_block_by_hash(block_hash, blk);
  m_blockchain_storage->get_block_by_hash(blk.prev_id, prev_blk);
  uint64_t late_timestamp = blk.timestamp;
  uint64_t early_timestamp = prev_blk.timestamp;
  
  std::vector<exchange_trade> result;
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].date <= late_timestamp && trades[i].date >= early_timestamp)
    {
      result.push_back(trades[i]);
    }
  }
  
  return result;
}


double get_usd_average(double gemini_usd, double coinbase_pro_usd){
  return (gemini_usd + coinbase_pro_usd) / 2;
}

uint64_t convert_btc_to_usd(double btc)
{
	double usd_average = get_usd_average(get_gemini_btc_usd(), get_coinbase_pro_btc_usd());
	double usd = usd_average * btc;
	return static_cast<uint64_t>(usd * 100); // remove "cents" decimal place and convert to integer
}

double price_over_x_blocks(unsigned int blocks){
  double ribbon_blue_sum = 0;
  uint64_t top_block_height = m_blockchain_storage->get_current_blockchain_height() - 1;

  for(size_t i = 1; i > blocks - blocks;i++){
    uint64_t this_top_block_height = m_blockchain_storage->get_current_blockchain_height() - i;
    crypto::hash this_block_hash = m_blockchain_storage->get_block_id_by_height(this_top_block_height);
    cryptonote::block this_blk;
    m_blockchain_storage->get_block_by_hash(this_block_hash, this_blk);
    std::string::size_type size;
    double blk_rb = 0;
    ribbon_blue_sum += blk_rb;
  }

  return ribbon_blue_sum / blocks;
}

uint64_t create_ribbon_red(uint64_t height){
  uint64_t ma1_sum = 0;
  for (size_t i = 1; i <= 960; i++)
  {
    cryptonote::block blk;
    crypto::hash block_hash = m_blockchain_storage->get_block_id_by_height(height - i);
    m_blockchain_storage->get_block_by_hash(block_hash, blk);
    ma1_sum += blk.ribbon_blue;
  }
  uint64_t ma1 = ma1_sum / 960;
  
  uint64_t ma2_sum = 0;
  for (size_t i = 1; i <= 480; i++)
  {
    cryptonote::block blk;
    crypto::hash block_hash = m_blockchain_storage->get_block_id_by_height(height - i);
    m_blockchain_storage->get_block_by_hash(block_hash, blk);
    ma2_sum += blk.ribbon_blue;
  }
  uint64_t ma2 = ma2_sum / 480;
  
  uint64_t ma3_sum = 0;
  for (size_t i = 1; i <= 240; i++)
  {
    cryptonote::block blk;
    crypto::hash block_hash = m_blockchain_storage->get_block_id_by_height(height - i);
    m_blockchain_storage->get_block_by_hash(block_hash, blk);
    ma3_sum += blk.ribbon_blue;
  }
  uint64_t ma3 = ma3_sum / 240;
  
  uint64_t ma4_sum = 0;
  for (size_t i = 1; i <= 120; i++)
  {
    cryptonote::block blk;
    crypto::hash block_hash = m_blockchain_storage->get_block_id_by_height(height - i);
    m_blockchain_storage->get_block_by_hash(block_hash, blk);
    ma4_sum += blk.ribbon_blue;
  }
  uint64_t ma4 = ma4_sum / 120;
  
  return (ma1 + ma2 + ma3 + ma4) / 4;
}

uint64_t create_ribbon_blue(std::vector<exchange_trade> trades)
{
  double filtered_mean = filter_trades_by_deviation(trades);
  return convert_btc_to_usd(filtered_mean);
}

//Volume Weighted Average
uint64_t create_ribbon_green(std::vector<exchange_trade> trades){
  double weighted_mean = trades_weighted_mean(trades);
  return convert_btc_to_usd(weighted_mean);
}

//Volume Weighted Average with 2 STDEV trades removed
double filter_trades_by_deviation(std::vector<exchange_trade> trades)
{
  double weighted_mean = trades_weighted_mean(trades);
  int n = trades.size();
  double sum = 0;
  
  for (size_t i = 0; i < trades.size(); i++)
  {
    sum += pow((trades[i].price - weighted_mean), 2.0);
  }
  
  double deviation = sqrt(sum / (double)n);
  
  double max = weighted_mean + (2 * deviation);
  double min = weighted_mean - (2 * deviation);
  
  for (size_t i = 0; i < trades.size(); i++)
  {
    if (trades[i].price > max || trades[i].price < min)
      trades.erase(trades.begin() + i);
  }

  return trades_weighted_mean(trades);
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
