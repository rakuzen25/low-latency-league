#pragma once

#include <cstdint>
#include <list>
#include <vector>
#include <limits>

enum class Side : uint8_t
{
  BUY,
  SELL
};

using IdType = uint32_t;
using PriceType = uint16_t;
using QuantityType = uint16_t;

constexpr PriceType MAX_PRICE = std::numeric_limits<PriceType>::max();

// You CANNOT change this
struct Order
{
  IdType id; // Unique
  PriceType price;
  QuantityType quantity;
  Side side;
};

struct OrderLocation
{
  PriceType price;
  std::list<Order>::iterator it;
  bool isValid = false;
};

// You CAN and SHOULD change this
struct Orderbook
{
  std::vector<std::list<Order>> buyOrders;
  std::vector<std::list<Order>> sellOrders;
  std::vector<OrderLocation> orderLocations;

  Orderbook() : buyOrders(MAX_PRICE + 1), sellOrders(MAX_PRICE + 1), orderLocations(1e6) {}
};

extern "C"
{

  // Takes in an incoming order, matches it, and returns the number of matches
  // Partial fills are valid
  uint32_t match_order(Orderbook &orderbook, const Order &incoming);

  // Sets the new quantity of an order. If new_quantity==0, removes the order
  void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                          QuantityType new_quantity);

  // Returns total resting volume at a given price point
  uint32_t get_volume_at_level(Orderbook &orderbook, Side side,
                               PriceType price);

  // Performance of these do not matter. They are only used to check correctness
  Order lookup_order_by_id(Orderbook &orderbook, IdType order_id);
  bool order_exists(Orderbook &orderbook, IdType order_id);
  Orderbook *create_orderbook();
}
