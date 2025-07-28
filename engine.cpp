#include "engine.hpp"
#include <functional>
#include <optional>
#include <stdexcept>

// This is an example correct implementation
// It is INTENTIONALLY suboptimal
// You are encouraged to rewrite as much or as little as you'd like

uint32_t match_order(Orderbook &orderbook, const Order &incoming)
{
  uint32_t matchCount = 0;
  Order order = incoming; // Create a copy to modify the quantity

  if (order.side == Side::BUY)
  {
    // For a BUY, match with sell orders priced at or below the order's price.
    for (PriceType price = 0; price <= order.price && order.quantity > 0; ++price)
    {
      auto &ordersAtPrice = orderbook.sellOrders[price];
      for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && order.quantity > 0;)
      {
        QuantityType trade = std::min(order.quantity, orderIt->quantity);
        order.quantity -= trade;
        orderIt->quantity -= trade;
        ++matchCount;

        if (orderIt->quantity == 0)
        {
          orderbook.orderLocations[orderIt->id].isValid = false; // Mark the order as invalid
          orderIt = ordersAtPrice.erase(orderIt);
        }
        else
          ++orderIt;
      }
    }

    if (order.quantity > 0)
    {
      auto &orderList = orderbook.buyOrders[order.price];
      orderList.push_back(order);

      auto it = std::prev(orderList.end()); // Get the iterator to the newly added order
      orderbook.orderLocations[order.id] = {order.price, it, true};
    }
  }
  else
  { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    for (PriceType price = MAX_PRICE; price >= order.price && order.quantity > 0; --price)
    {
      auto &ordersAtPrice = orderbook.buyOrders[price];
      for (auto orderIt = ordersAtPrice.begin(); orderIt != ordersAtPrice.end() && order.quantity > 0;)
      {
        QuantityType trade = std::min(order.quantity, orderIt->quantity);
        order.quantity -= trade;
        orderIt->quantity -= trade;
        ++matchCount;

        if (orderIt->quantity == 0)
        {
          orderbook.orderLocations[orderIt->id].isValid = false; // Mark the order as invalid
          orderIt = ordersAtPrice.erase(orderIt);
        }
        else
          ++orderIt;
      }
    }

    if (order.quantity > 0)
    {
      auto &orderList = orderbook.sellOrders[order.price];
      orderList.push_back(order);

      auto it = std::prev(orderList.end()); // Get the iterator to the newly added order
      orderbook.orderLocations[order.id] = {order.price, it, true};
    }
  }
  return matchCount;
}

void modify_order_by_id(Orderbook &orderbook, IdType order_id,
                        QuantityType new_quantity)
{
  if (order_id >= orderbook.orderLocations.size() || !orderbook.orderLocations[order_id].isValid)
    return; // Order does not exist or is invalid

  OrderLocation &loc = orderbook.orderLocations[order_id];
  if (new_quantity == 0)
  {
    auto &orderList = (loc.it->side == Side::BUY ? orderbook.buyOrders[loc.price] : orderbook.sellOrders[loc.price]);
    orderList.erase(loc.it);
    loc.isValid = false; // Mark the order as invalid
  }
  else
  {
    // Modify the order's quantity
    loc.it->quantity = new_quantity;
  }
}

uint32_t get_volume_at_level(Orderbook &orderbook, Side side,
                             PriceType price)
{
  uint32_t total = 0;
  const auto &ordersAtPrice =
      (side == Side::BUY ? orderbook.buyOrders : orderbook.sellOrders)[price];

  for (const auto &order : ordersAtPrice)
  {
    total += order.quantity;
  }
  return total;
}

// Functions below here don't need to be performant. Just make sure they're
// correct
Order lookup_order_by_id(Orderbook &orderbook, IdType order_id)
{
  if (!order_exists(orderbook, order_id))
    throw std::out_of_range("Order ID out of range or invalid");

  return *orderbook.orderLocations[order_id].it;
}

bool order_exists(Orderbook &orderbook, IdType order_id)
{
  return order_id < orderbook.orderLocations.size() && orderbook.orderLocations[order_id].isValid;
}

Orderbook *create_orderbook() { return new Orderbook; }
