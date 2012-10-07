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

#ifndef __XQUEUE_HH_INCLUDED__
#define __XQUEUE_HH_INCLUDED__

#include <functional> // For std::less
#include <utility>    // For std::pair
#include <vector>     // For std::vector
#include <stdexcept>  // For std::range_error and std::underflow_error

namespace xq
{

  //
  //      Type: the value type
  //  Comparer: comparer for Type
  // Allocator: template allocator class, will take
  //            std::pair<Type, xpq::position<queue> >
  //            as its template argument
  // Container: template container class, default to std::vector
  //
  template <typename Type,
    typename Comparer = std::less<Type>,
    template<typename T> class Allocator = std::allocator,
    template<typename T, typename A> class Container = std::vector>
    class xqueue
    {
      // Because of the complicated dependency for following defininition,
      // the code have to be splited.
    public:
      class handle;
    private:
      typedef std::pair<Type, handle*> Element;
    public:

      typedef Container<Element, Allocator<Element> > container;

      class handle
      {
        friend class xqueue;
      public:
        typedef typename container::difference_type difference_type;
        handle()
          : m_offset(-1)
        { }

        ~handle()
        { }

      private:
        difference_type m_offset;
      };

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
        m_container.clear();
      }

      // Insert a value into queue and return its position
      void insert(handle &h, const Type &value)
      {
        if (h.m_offset >= 0)
          throw std::runtime_error("Handle already used");
        m_container.insert(m_container.end(), Element(value, &h));
        // We don't need to set the m_offset member of the element,
        // as push_heap will do it well.
        push_heap(m_container.begin(), m_container.end());
      }
#if __cplusplus >= 201103L
      // This will require Type has move constructor
      void insert(handle &h, Type &&value)
      {
        if (h.m_offset >= 0)
          throw std::runtime_error("Handle already used");
        m_container.insert(m_container.end(), Element(value, &h));
        push_heap(m_container.begin(), m_container.end());
      }
#endif

      // Return first one in the queue
      handle &front()
      {
        if (!size())
          throw std::underflow_error("Queue is empty");
        return *m_container.begin()->second;
      }

      // Extract a value for a given position, and remove it from the queue
      Type remove(handle &h)
      {
        iterator i = m_container.begin() + h.m_offset;

        if (!check_range(m_container.begin(), m_container.end(), i))
          throw std::range_error("Handle invalid");

        pop_heap(m_container.begin(), m_container.end(), i);
#if __cplusplus >= 201103L
        Type ret = std::move(i->first);
#else
        Type ret = i->first;
#endif
        h.m_offset = -1;
        m_container.pop_back();
        return ret;
      }

      // Update the value of given position
      void update(handle &h, const Type &value)
      {
        iterator origin = m_container.begin() + h.m_offset;
        
        if (!check_range(m_container.begin(), m_container.end(), origin))
          throw std::range_error("Handle invalid");

        // m_comparer compares Element, while m_comparer.m_comparer
        // compares Type
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
        // else new value is equals to origin
      }

#if __cplusplus >= 201103L

      // Update the value of given position
      void update(handle &h, Type &&value)
      {
        iterator origin = m_container.begin() + h.m_offset;

        if (!check_range(m_container.begin(), m_container.end(), origin))
          throw std::range_error("Handle invalid");
          
        if (m_comparer.m_comparer(value, origin->first))
        {
          origin->first = move(value);
          push_heap(m_container.begin(), origin + 1);
        }
        else if (m_comparer.m_comparer(origin->first, value))
        {
          origin->first = move(value);
          adjust_heap(m_container.begin(), m_container.end(), origin);
        }
        // else new value is equals to origin
      }
#endif

    private:
      typedef typename container::iterator iterator;
      typedef typename container::difference_type difference_type;

#if __cplusplus < 201103L
      inline const Element &move(Element &value)
      {
        return value;
      }
#else
      inline Element &&move(Element &value)
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

      // Wrap Type comparer as Element comparer
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

