#pragma once

template<typename T, T * (T::*Next) () const, void (T::*Set_Next)(T *)>
class hash_bucket 
{
    T * start = nullptr;

public:
    hash_bucket () {}

    template <typename Predicate>
    auto find (Predicate condition) -> T *
    {
        auto p = start;

        while (p)
        {
            if (condition (*p))
                break;

            p = (p->*Next) ();
        }
        return p;
    }

    template <typename Function>
    auto for_each (Function &&apply) -> void
    {
        auto p = start;
        while (p)
        {
            apply (*p);
            p = (p->*Next) ();
        }
    }

    void prepend (T &item)
    {
        (item.*Set_Next) (start);
        start = &item;
    }

    void remove (T &item)
    {
        if (start == &item)
        {
            start = (item.*Next) ();
        }
        else
        {
            auto q = start;
            while ((q->*Next) () != &item) { q = (q->*Next) (); }
            (q->*Set_Next) ((item.*Next)());
        }
    }
};

