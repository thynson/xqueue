/*
 * Copyright (C) 2012 LAN Xingcan
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __XPQ_HH_INCLUDED__
#define __XPQ_HH_INCLUDED__

#include <functional> // For std::less
#include <utility>    // For std::pair
#include <vector>     // For std::vector
#include <stdexcept>  // For std::range_error and std::underflow_error

namespace xq
{
  // Forward declarations

  //
  // Queue: the type of xqueue
  //
  template <typename Queue>
    class position;

  //
  //      Type: the value type
  //  Comparer: comparer for Type
  // Allocator: template allocator class, for will take
  //            std::pair<Type, xpq::position<queue> >
  //            as its template argument
  // Container: template container class, default to std::vector
  //
  template <typename Type, typename Comparer,
    template<typename T> class Allocator,
    template<typename T, typename A> class Container>
    class xqueue;

  template <typename Type, typename Comparer,
    template<typename T> class Allocator,
    template<typename T, typename A> class Container>
    class position<xqueue<Type, Comparer, Allocator, Container> >
    {
    private:
      typedef xqueue<Type, Comparer, Allocator, Container> Queue;
      friend class xqueue<Type, Comparer, Allocator, Container>;

    public:
      position()
        : m_offset()
      { }

      ~position()
      { }

    public:

      typedef typename Queue::container container;
      typedef typename container::difference_type difference_type;
      typedef typename container::iterator iterator;

      difference_type m_offset;
    };


  template <typename Type,
    typename Comparer = std::less<Type>,
    template<typename T> class Allocator = std::allocator,
    template<typename T, typename A> class Container = std::vector>
    class xqueue
    {
      // Because of the complicated dependency for following defininition,
      // the code have to be splited.
    private:
      friend class position<xqueue>;
    public:
      typedef position<xqueue> position_t;
    private:
      typedef std::pair<Type, position_t*> Element;
    public:
      typedef Container<Element, Allocator<Element> > container;
      typedef typename container::size_type size_type;

      xqueue()
        : m_container()
        , m_comparer()
      { }

      ~xqueue()
      {
        clear();
      }

      size_type size()
      {
        return m_container.size();
      }

      bool empty()
      {
        return !size();
      }

      void clear()
      {
        for (iterator i = m_container.begin(); i < m_container.end(); ++i)
          i->second->m_offset = -1;
      }

      // Push a value into queue and return its position
      void push(const Type &value, position_t &pos)
      {
        pos.m_offset = m_container.insert(m_container.end(),
                       make_pair(value, &pos)) - m_container.begin();
        push_heap(m_container.begin(), m_container.end());
      }

      // Return first one in the queue
      position_t &front()
      {
        if (!size())
          throw std::underflow_error("Queue is empty");
        return *m_container.begin()->second;
      }

      // Extract a value for a given position, and remove it from the queue
      Type remove(position_t &p)
      {
        iterator pos = m_container.begin() + p.m_offset;

        if (!check_range(m_container.begin(), m_container.end(), pos))
          throw std::range_error("Position invalid");

        pop_heap(m_container.begin(), m_container.end(), pos);
        Type ret = pos->first;
        p.m_offset = -1;
        m_container.pop_back();
        return ret;
      }

      // Update the value of given position
      void update(position_t &pos, const Type &value)
      {
        iterator origin = m_container.begin() + pos.m_offset;
        if (m_comparer.m_comparer(value, origin->first))
        {
          origin->first = value;
          push_heap(m_container.begin(), origin + 1);
        }
        else if (m_comparer.m_comparer(origin->first, value))
        {
          origin->first = value;
          adjust_heap(m_container.begin(), m_container.end(), origin);
        }
      }

    private:
      typedef typename container::iterator iterator;
      typedef typename container::difference_type difference_type;

#if __cplusplus < 201103L
      inline const Element & move(Element &value)
      {
        return value;
      }
#else
      inline Element &&move(Element &value)
      {
        return std::move(value);
      }

      inline Element &&move(Element &&value)
      {
        return std::move(value);
      }
#endif

      bool check_range(iterator begin, iterator end, iterator pos)
      {
        return begin <= pos && pos < end;
      }


      void adjust_heap(iterator begin, iterator end, iterator pos)
      {
        Element tmp = move(*pos);
        iterator i;
        for ( ; ; )
        {
          difference_type diff = pos - begin;
          iterator left_child = begin + diff * 2 + 1;
          iterator right_child = begin + (diff + 1) * 2;
          i = pos;

          if (left_child < end && m_comparer(*left_child, tmp))
            i = left_child;

          if (right_child < end && m_comparer(*right_child, *i))
            i = right_child;

          if (i != pos)
          {
            *pos = move(*i);
            pos->second->m_offset = pos - begin;
            pos = i;
          }
          else
            break;
        }
        *pos = move(tmp);
        pos->second->m_offset = pos - begin;
      }

      void push_heap(iterator begin, iterator end)
      {
        iterator i = end - 1;
        Element tmp = move(*i);

        while(i != begin)
        {
          iterator parent = begin + (i - begin - 1) / 2;
          if (!m_comparer(tmp, *parent))
            break;
          *i = move(*parent);
          i->second->m_offset = i - begin;
          i = parent;
        }
        *i = move(tmp);
        i->second->m_offset = i - begin;
      }

      void pop_heap(iterator begin, iterator end, iterator pos)
      {
        iterator last = end - 1;
        Element tmp = move(*pos);
        *pos = move(*last);
        pos->second->m_offset = pos - begin;
        adjust_heap(begin, last, pos);
        *last = move(tmp);
        last->second->m_offset = last - begin;
      }

      struct comparer_t
      {
        Comparer m_comparer;
        bool operator () (const Element &lhs, const Element &rhs)
        {
          return m_comparer(lhs.first, rhs.first);
        }
      };
      container m_container;
      comparer_t m_comparer;
    };
}


#endif

