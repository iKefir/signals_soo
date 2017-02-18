#ifndef signals_soo_hpp
#define signals_soo_hpp

#include <functional>
#include <list>
#include <memory>

template <typename R>
struct my_signal;

template <typename...Params>
struct my_signal<void(Params...)>
{
    typedef std::function<void(Params...)> slot_t;
    typedef size_t id_t;
    
    struct connection_item
    {
        connection_item() = default;
        
        connection_item(my_signal* par, slot_t sl, bool cnct)
        : parent(par), slot(sl), connected(cnct)
        {}
        
        void disconnect() {
            connected = false;
        }
        
        bool is_connected() const {
            return connected;
        }
        
        void operator()(Params ...p) const {
            slot(p...);
        }
        
    private:
        my_signal* parent;
        slot_t slot;
        bool connected;
    };
    
    typedef std::shared_ptr<connection_item> ptr_t;
    
    struct connection
    {
        connection(my_signal* par, ptr_t ptr)
        : parent(par), pointer(ptr)
        {}
        
        void disconnect()
        {
            parent -> do_disconnect(pointer);
        }
        
    private:
        my_signal* parent;
        ptr_t pointer;
    };
    
    my_signal()
    : count(0), small(), slots(), entrancy(false)
    {
        to_add = std::list<ptr_t>();
        to_rm  = std::list<ptr_t>();
    }
    
    connection connect(slot_t slot)
    {
        ptr_t ptr = std::make_shared<connection_item>(this, slot, true);
        if (!entrancy) {
            if (is_small()) {
                if (count == 1) {
                    slots = std::list<ptr_t>();
                    slots.emplace_back(small);
                    slots.push_back(ptr);
                } else {
                    small = ptr;
                }
            } else {
                slots.push_back(ptr);
            }
            ++count;
        } else {
            to_add.push_back(ptr);
        }
        return connection(this, ptr);
    }
    
    
    void disconnect_all_slots() {
        if (is_small()) {
            do_disconnect(small);
        } else {
            for (auto it = slots.begin(); it != slots.end(); it++) {
                do_disconnect((*it));
            }
        }
    }
    
    void do_disconnect(ptr_t ptr) {
        if (!entrancy) {
            ptr -> disconnect();
        } else {
            to_rm.push_back(ptr);
        }
    }
    
    void operator()(Params...p)
    {
        bool prev_entrancy = entrancy;
        entrancy = true;
        if (is_small()) {
            if (count == 1) {
                if ((*small).is_connected()) {
                    (*small)(std::forward<Params>(p)...);
                } else {
                    --count;
                }
            }
        } else {
            for (auto it = slots.cbegin(); it != slots.cend(); it++)
                if ((*it) -> is_connected()) {
                    (*(*it))(std::forward<Params>(p)...);
                }
            for (auto it = slots.begin(); it != slots.end(); it++) {
                if (!(*it) -> is_connected()) {
                    it = slots.erase(it);
                    if (it != slots.begin()) --it;
                    --count;
                }
            }
            if (count == 1) {
                small = slots.front();
                slots.clear();
            }
        }
        entrancy = prev_entrancy;
        
        for (auto it = to_rm.begin(); it != to_rm.end(); it++) {
            (*it) -> disconnect();
        }
        
        to_rm.clear();
        
        for (auto it = to_add.begin(); it != to_add.end(); it++) {
            slots.emplace_back(std::move(*it));
        }
        
        to_add.clear();
    }
    
    inline bool is_small() const {
        return count < 2;
    }
    
private:
    id_t count;
    ptr_t small;
    bool entrancy;
    std::list<ptr_t> slots;
    
    std::list<ptr_t> to_add;
    std::list<ptr_t> to_rm;
    
};

#endif /* signals_soo_hpp */
