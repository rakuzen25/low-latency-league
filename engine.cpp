#include "engine.hpp"
#include <functional>
#include <optional>
#include <stdexcept>

// This is an example correct implementation
// It is INTENTIONALLY suboptimal
// You are encouraged to rewrite as much or as little as you'd like

void remove_order(Orderbook &orderbook, Side side, PriceType price, size_t index)
{
  auto &ordersAtPrice = (side == Side::BUY ? orderbook.buyOrders : orderbook.sellOrders)[price];

  orderbook.orderLocations[ordersAtPrice[index].id].isValid = false; // Mark the order as invalid

  if (ordersAtPrice.size() > 1 && index < ordersAtPrice.size() - 1)
  {
    // Swap with the last element
    Order last = ordersAtPrice.back();
    ordersAtPrice[index] = last;
    orderbook.orderLocations[last.id].index = index;
  }

  ordersAtPrice.pop_back(); // Remove the last element

  if (ordersAtPrice.empty())
  {
    if (side == Side::SELL && price == orderbook.minSellPrice)
    {
      for (PriceType p = price + 1; p < MAX_PRICE; ++p)
      {
        if (!orderbook.sellOrders[p].empty())
        {
          orderbook.minSellPrice = p;
          return;
        }
      }
      orderbook.minSellPrice = MAX_PRICE;
    }
    else if (side == Side::BUY && price == orderbook.maxBuyPrice)
    {
      for (PriceType p = price - 1; p > 0; --p)
      {
        if (!orderbook.buyOrders[p].empty())
        {
          orderbook.maxBuyPrice = p;
          return;
        }
      }
      orderbook.maxBuyPrice = 0;
    }
  }
}

uint32_t match_order(Orderbook &orderbook, const Order &incoming)
{
  uint32_t matchCount = 0;
  Order order = incoming; // Create a copy to modify the quantity

  if (order.side == Side::BUY)
  {
    // For a BUY, match with sell orders priced at or below the order's price.
    for (PriceType price = orderbook.minSellPrice; price <= order.price && order.quantity > 0; ++price)
    {
      auto &ordersAtPrice = orderbook.sellOrders[price];
      if (ordersAtPrice.empty())
        continue; // No orders at this price, skip

      for (size_t i = 0; i < ordersAtPrice.size() && order.quantity > 0;)
      {
        Order &resting = ordersAtPrice[i];
        QuantityType trade = std::min(order.quantity, resting.quantity);
        if (trade > 0)
        {
          order.quantity -= trade;
          resting.quantity -= trade;
          matchCount++;
        }

        if (resting.quantity == 0)
        {
          remove_order(orderbook, Side::SELL, price, i);
        }
        else
        {
          // Move to the next order only if we didn't remove the current one
          ++i;
        }
      }
    }

    if (order.quantity > 0)
    {
      auto &orderList = orderbook.buyOrders[order.price];
      orderList.push_back(order);
      orderbook.orderLocations[order.id] = {order.side, order.price, orderList.size() - 1, true};
      if (order.price > orderbook.maxBuyPrice)
      {
        orderbook.maxBuyPrice = order.price; // Update max buy price if necessary
      }
    }
  }
  else
  { // Side::SELL
    // For a SELL, match with buy orders priced at or above the order's price.
    for (PriceType price = orderbook.maxBuyPrice; price >= order.price && order.quantity > 0; --price)
    {
      auto &ordersAtPrice = orderbook.buyOrders[price];
      if (ordersAtPrice.empty())
        continue; // No orders at this price, skip

      for (size_t i = 0; i < ordersAtPrice.size() && order.quantity > 0;)
      {
        Order &resting = ordersAtPrice[i];
        QuantityType trade = std::min(order.quantity, resting.quantity);
        if (trade > 0)
        {
          order.quantity -= trade;
          resting.quantity -= trade;
          matchCount++;
        }

        if (resting.quantity == 0)
        {
          remove_order(orderbook, Side::BUY, price, i);
        }
        else
        {
          // Move to the next order only if we didn't remove the current one
          ++i;
        }
      }
      if (price == 0)
        break;
    }

    if (order.quantity > 0)
    {
      auto &orderList = orderbook.sellOrders[order.price];
      orderList.push_back(order);
      orderbook.orderLocations[order.id] = {order.side, order.price, orderList.size() - 1, true};
      if (order.price < orderbook.minSellPrice)
      {
        orderbook.minSellPrice = order.price; // Update min sell price if necessary
      }
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
    remove_order(orderbook, loc.side, loc.price, loc.index);
  }
  else
  {
    auto &ordersAtPrice = (loc.side == Side::BUY ? orderbook.buyOrders : orderbook.sellOrders)[loc.price];
    ordersAtPrice[loc.index].quantity = new_quantity;
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
    throw std::runtime_error("Order does not exist");

  const auto &loc = orderbook.orderLocations[order_id];
  const auto &ordersAtPrice =
      (loc.side == Side::BUY ? orderbook.buyOrders : orderbook.sellOrders)[loc.price];
  return ordersAtPrice[loc.index];
}

bool order_exists(Orderbook &orderbook, IdType order_id)
{
  return order_id < orderbook.orderLocations.size() && orderbook.orderLocations[order_id].isValid;
}

Orderbook *create_orderbook() { return new Orderbook; }
