/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301  USA
 */

%{
#include <list>
%}

namespace std {

    template<class T> class list {
      public:
        typedef size_t size_type;
        typedef T value_type;
        typedef const value_type& const_reference;
        list();
        list(size_type n);
        size_type size() const;
        %rename(isEmpty) empty;
        bool empty() const;
        void clear();
        void reverse();
        %rename(addFirst) push_front;
        void push_front(const value_type& x);
        %rename(addLast) push_back;
        void push_back(const value_type& x);
        %rename(getFirst) front;
        const_reference front();
        %rename(getLast) back;
        const_reference back();
        %rename(clearLast) pop_back;
        void pop_back();
        %rename(clearFirst) pop_front;
        void pop_front();
        void remove(const value_type& x);
   };
}
