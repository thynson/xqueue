
#ifndef __XPQ_HH_INCLUDED__
#define __XPQ_HH_INCLUDED__

#include <functional> // For std::less
#include <utility>    // For std::pair
#include <vector>     // For std::vector

namespace xpq
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
        : m_position()
      { }

      ~position()
      { }

    public:

      typedef typename Queue::container container;
      typedef typename container::iterator iterator;

      iterator m_position;
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

      xqueue()
        : m_container()
        , m_comparer()
      { }

      ~xqueue()
      { }

      // Push a value into queue and return its position
      void push(const Type &value, position_t &pos)
      {
        pos.m_position = m_container.insert(m_container.end(),
                                            make_pair(value, &pos));
        push_heap(m_container.begin(), m_container.end());
      }

      // Return first one in the queue
      position_t &front()
      {
        return *m_container.begin()->second;
      }

      // Extract a value for a given position, and remove it from the queue
      Type remove(position_t &p)
      {
        heap_pop(m_container.begin(), m_container.end(), p.m_position);
        Type ret = p.m_position->first;
        p.m_position = m_container.end();
        m_container.pop_back();
        return ret;
      }

      // Update the value of given position
      void update(position_t &pos, const Type &value)
      {
        if (m_comparer(value, pos.m_container->first))
        {
          pos.m_container->first = value;
          push_heap(m_container.begin(), pos.m_container + 1);
        }
        else
        {
          pos.m_container->first = value;
          adjust_heap(m_container.begin(), m_container.end(), pos.m_position);
        }
      }

    private:
      typedef typename container::iterator iterator;
      typedef typename container::difference_type difference_type;

      void adjust_heap(iterator begin, iterator end, iterator pos)
      {
        Type tmp = pos->first;
        iterator i;
        for ( ; ; )
        {
          difference_type diff = pos - begin;
          iterator left_child = begin + diff * 2 + 1;
          iterator right_child = begin + (diff + 1) * 2;
          i = pos;

          if (left_child < end && m_comparer(*left_child, *pos))
            i = left_child;

          if (right_child < end && m_comparer(*right_child, *i))
            i = right_child;

          if (i != pos)
          {
            pos->first = i->first;
            pos->second->m_position = pos;
          }
          else
            return;
        }
      }

      void push_heap(iterator begin, iterator end)
      {
        iterator i = end - 1;
        Type tmp = i->first;

        while(i != begin)
        {
          iterator parent = begin + (i - begin - 1) / 2;
          if (!m_comparer(*i, *parent))
            break;
          i->first = parent->first;
          i->second->m_position = i;
          i = parent;
        }
        i->first = tmp;
        i->second->m_position = i;
      }

      void heap_pop(iterator begin, iterator end, iterator pos)
      {
        iterator last = end - 1;
        Type tmp = pos->first;
        pos->first = last->first;
        pos->second->m_position = pos;
        adjust_heap(begin, last, pos);
        last->first = tmp;
        last->second->m_position = end;
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

