namespace ListPlex
{
    template <typename T>
    struct Stack
    {
        T *members;
        int sz;
        int cap;

        Stack() : members(nullptr), sz(0), cap(0) {}
        //Stack(T *_list, int _sz) : sz(_sz), members(list) {}
        //emty stack
        Stack(int _cap) : sz(0), cap(_cap)
        {
            members = new T[cap];            
        }
        Stack(const Stack &_stk) : sz(_stk.sz), cap(_stk.cap)
        {
            members = new T[cap];
            memcpy(members, _stk.members, sizeof(T) * sz);
        }
        // A child restores only its own stack frames; retain the live prefix.
        Stack(const Stack& source,int universe) : sz(source.sz),cap(universe) {
            assert(universe>=sz && universe<=source.cap);
            members=new T[cap];
            if (sz) memcpy(members,source.members,sizeof(T)*sz);
        }
        ~Stack() { delete[] members; }

        inline void push(const T &v)
        {
            assert(sz < cap);
            members[sz++] = v;
        }
        inline void pop()
        {
            assert(sz > 0);
            sz--;
        }
        inline T top()
        {
            assert(sz > 0);
            return members[sz - 1];
        }

        template <typename Func>
        void for_each(Func &&f)
        {
            for (int i = 0; i < sz; i++)
            {
                f(members[i]);
            }
        }

        inline void clear(){
            sz = 0;
        }

        inline void resize(const int _cap){
            if (_cap <= cap) return;
            T *grown = new T[_cap];
            if (sz) memcpy(grown, members, sizeof(T) * sz);
            delete[] members;
            members = grown;
            cap = _cap;
        }
    };
}