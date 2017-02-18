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
        connection(ptr_t ptr)
        : pointer(ptr)
        {}
        
        void disconnect()
        {
            pointer -> disconnect();
        }
        
    private:
        ptr_t pointer;
    };
    
    my_signal()
    : count(0), small(), slots()
    {}
    
    connection connect(slot_t slot)
    {
        ptr_t ptr = std::make_shared<connection_item>(this, slot, true);
        if (is_small()) {
            if (count == 1) {
                slots = std::make_shared<std::list<ptr_t>>();
                (*slots).emplace_back(small);
                (*slots).push_back(ptr);
            } else {
                small = ptr;
            }
        } else {
            (*slots).push_back(ptr);
        }
        ++count;
        return connection(ptr);
    }
    
    void disconnect_all_slots() {
        for (auto& conn : slots) {
            conn -> disconnect();
        }
    }
    
    void operator()(Params...p)
    {
        if (is_small()) {
            if (count == 1) {
                if ((*small).is_connected()) {
                    (*small)(p...);
                } else {
                    --count;
                }
            }
        } else {
            for (auto it = (*slots).cbegin(); it != (*slots).cend(); it++)
                if ((*it) -> is_connected()) {
                    (*(*it))(p...);
                }
            for (auto it = (*slots).begin(); it != (*slots).end(); it++) {
                if (!(*it) -> is_connected()) {
                    (*slots).erase(it);
                    --count;
                }
            }
            if (count == 1) {
                small = (*slots).front();
                slots.reset();
            }
        }
    }
    
    inline bool is_small() const {
        return count < 2;
    }
    
private:
    id_t count;
    ptr_t small;
    std::shared_ptr<std::list<ptr_t>> slots;
    
};

#endif /* signals_soo_hpp */
