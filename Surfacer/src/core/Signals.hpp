//
//  Signals.h
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#ifndef Signals_h
#define Signals_h

#include <map>
#include <set>
#include <vector>
#include <functional>
#include <type_traits>


namespace core {
    namespace signals {

        using std::placeholders::_1;
        using std::placeholders::_2;
        using std::placeholders::_3;
        using std::size_t;


        class receiver;

        namespace detail {

            typedef size_t free_function_id;

            class slot_registry {
            protected:

                std::set<receiver *> _receivers;

            public:

                void add(receiver *r) {
                    _receivers.insert(r);
                }

                void remove(receiver *r) {
                    _receivers.erase(r);
                }

                bool has(receiver *r) const {
                    return _receivers.count(r);
                }

                const std::set<receiver *> &receivers() const {
                    return _receivers;
                }

            };

            void add_registry_to_receiver(slot_registry *, receiver *);

            void remove_registry_from_receiver(slot_registry *, receiver *);

            bool receiver_is_destructing(receiver *);
        }

#pragma mark -
#pragma mark receiver

        /**
            base classes for objects receiving signals, which want to be automatically
            disconnected from a signal when they're destroyed.
         */

        class receiver {
        public:

            receiver() :
                    _destructing(false) {
            }

            virtual ~receiver() {
                _destructing = true;
                for (std::set<detail::slot_registry *>::iterator reg(_registries.begin()), end(_registries.end()); reg != end; ++reg) {
                    (*reg)->remove((receiver *) (this));
                }
            }

        private:

            friend void detail::add_registry_to_receiver(detail::slot_registry *, receiver *);

            friend void detail::remove_registry_from_receiver(detail::slot_registry *, receiver *);

            friend bool detail::receiver_is_destructing(receiver *);

            bool _destructing;
            std::set<detail::slot_registry *> _registries;

        };


#pragma mark -
#pragma mark signal

        /**

         signal

         Base class for emitting signals. Define a signal with its signature, e.g., signals::signal<void(int)> to define a signal which passes a single int.

         Up to three arguments are supported. Emitting the signal is done via the () operator.

         Free functions and methods on objects are supported.

         */
        template<typename Signature>
        class signal {
        public:

            typedef std::function<Signature> callback_type;

            typedef std::vector<std::pair<void *, callback_type> > method_vec;
            typedef typename method_vec::value_type method_vec_value_type;
            typedef typename method_vec::iterator method_vec_iterator;

            typedef std::vector<std::pair<detail::free_function_id, callback_type> > function_vec;
            typedef typename function_vec::value_type function_vec_value_type;
            typedef typename function_vec::iterator function_vec_iterator;

            signal() {
            }

            ~signal() {
                //
                //	go through connected receivers in our registry and remove our registry from each.
                //	this is to handle the case where this emitter is deleted before receivers connected to it -
                //	we don't want their destructors talking to our deleted signal_registry.
                //

                for (std::set<receiver *>::iterator it(_registry.receivers().begin()), end(_registry.receivers().end()); it != end; ++it) {
                    receiver *rec = (receiver *) (*it);
                    detail::remove_registry_from_receiver(&_registry, rec);
                }
            }

            /**
             connect to a method taking zero parameters
                */
            template<class T>
            void connect(T *obj, void (T::*method)()) {
                _connectMethodCallback(obj, std::bind(method, obj));
            }

            /**
             connect to a method taking 1 parameter
                */
            template<class T, class A>
            void connect(T *obj, void (T::*method)(A)) {
                _connectMethodCallback(obj, std::bind(method, obj, _1));
            }

            /**
             connect to a method taking 2 parameters
                */
            template<class T, class A, class B>
            void connect(T *obj, void (T::*method)(A, B)) {
                _connectMethodCallback(obj, std::bind(method, obj, _1, _2));
            }

            /**
             connect to a method taking 3 parameters
                */
            template<class T, class A, class B, class C>
            void connect(T *obj, void (T::*method)(A, B, C)) {
                _connectMethodCallback(obj, std::bind(method, obj, _1, _2, _3));
            }

            /**
             disconnect any method connections made to an object
                */
            template<class T>
            void disconnect(T *obj) {
                void *objId = (void *) obj;

                if (std::is_convertible<T *, receiver *>::value) {
                    _safeMethods.erase(std::remove_if(_safeMethods.begin(), _safeMethods.end(), same_receiver(objId)),
                            _safeMethods.end());

                    receiver *rec = static_cast<receiver *>(obj);
                    detail::remove_registry_from_receiver(&_registry, rec);
                    _registry.remove(rec);
                } else {
                    _methods.erase(std::remove_if(_methods.begin(), _methods.end(), same_receiver(objId)),
                            _methods.end());
                }
            }

            /**
             connect a free function taking zero parameters
                */
            void connect(void (*function)()) {
                size_t fId = (size_t) function;
                _functions.push_back(function_vec_value_type(fId, std::bind(function)));
            }

            /**
             connect a free function taking 1 parameter
                */
            template<class A>
            void connect(void (*function)(A)) {
                size_t fId = (size_t) function;
                _functions.push_back(function_vec_value_type(fId, std::bind(function, _1)));
            }

            /**
             connect a free function taking 2 parameters
                */
            template<class A, class B>
            void connect(void (*function)(A, B)) {
                size_t fId = (size_t) function;
                _functions.push_back(function_vec_value_type(fId, std::bind(function, _1, _2)));
            }

            /**
             connect a free function taking 3 parameters
                */
            template<class A, class B, class C>
            void connect(void (*function)(A, B, C)) {
                size_t fId = (size_t) function;
                _functions.push_back(function_vec_value_type(fId, std::bind(function, _1, _2, _3)));
            }

            /**
             disconnect a free function taking zero parameters
                */
            void disconnect(void (*function)()) {
                size_t fId = (size_t) function;
                _functions.erase(
                        std::remove_if(_functions.begin(), _functions.end(), same_function(fId)),
                        _functions.end());
            }

            /**
             disconnect a free function taking 1 parameter
                */
            template<class A>
            void disconnect(void (*function)(A)) {
                size_t fId = (size_t) function;
                _functions.erase(
                        std::remove_if(_functions.begin(), _functions.end(), same_function(fId)),
                        _functions.end());
            }

            /**
             disconnect a free function taking 2 parameters
                */
            template<class A, class B>
            void disconnect(void (*function)(A, B)) {
                size_t fId = (size_t) function;
                _functions.erase(
                        std::remove_if(_functions.begin(), _functions.end(), same_function(fId)),
                        _functions.end());
            }

            /**
             disconnect a free function taking 3 parameters
                */
            template<class A, class B, class C>
            void disconnect(void (*function)(A, B, C)) {
                size_t fId = (size_t) function;
                _functions.erase(
                        std::remove_if(_functions.begin(), _functions.end(), same_function(fId)),
                        _functions.end());
            }

            /**
             Invoke this signal, passing zero parameters
                */
            void operator()() {
                _invoke(invoker_0());
            }

            /**
             Invoke this signal, passing 1 parameter
                */
            template<typename A>
            void operator()(A &a) {
                _invoke(invoker_1<A>(a));
            }

            /**
             Invoke this signal, passing 1 parameter
                */
            template<typename A>
            void operator()(const A &a) {
                _invoke(invoker_1_const<A>(a));
            }

            /**
             Invoke this signal, passing 2 parameters
                */
            template<typename A, typename B>
            void operator()(A &a, B &b) {
                _invoke(invoker_2<A, B>(a, b));
            }

            /**
             Invoke this signal, passing 2 parameters
                */
            template<typename A, typename B>
            void operator()(const A &a, const B &b) {
                _invoke(invoker_2_const<A, B>(a, b));
            }

            /**
             Invoke this signal, passing 3 parameters
                */
            template<typename A, typename B, typename C>
            void operator()(A &a, B &b, C &c) {
                _invoke(invoker_3<A, B, C>(a, b, c));
            }

            /**
             Invoke this signal, passing 3 parameters
                */
            template<typename A, typename B, typename C>
            void operator()(const A &a, const B &b, const C &c) {
                _invoke(invoker_3_const<A, B, C>(a, b, c));
            }

            bool empty() const {
                return _safeMethods.empty() && _methods.empty() && _functions.empty();
            }

        private:

            struct same_receiver {
                void *target;

                same_receiver(void *t) :
                        target(t) {
                }

                bool operator()(const method_vec_value_type &a) const {
                    return a.first == target;
                }
            };

            struct same_receivers {
                const std::vector<receiver *> &receivers;

                same_receivers(const std::vector<receiver *> &R) :
                        receivers(R) {
                }

                bool operator()(const method_vec_value_type &a) const {
                    return std::find(receivers.begin(), receivers.end(), a.first) != receivers.end();
                }
            };

            struct same_function {
                detail::free_function_id functionId;

                same_function(detail::free_function_id fId) :
                        functionId(fId) {
                }

                bool operator()(const function_vec_value_type &a) const {
                    return a.first == functionId;
                }
            };

            struct invoker_0 {
                void operator()(callback_type &c) const {
                    c();
                }
            };

            template<typename A>
            struct invoker_1 {
                A &_a;

                invoker_1(A &a) :
                        _a(a) {
                }

                void operator()(callback_type &c) const {
                    c(_a);
                }
            };

            template<typename A, typename B>
            struct invoker_2 {
                A &_a;
                B &_b;

                invoker_2(A &a, B &b) :
                        _a(a),
                        _b(b) {
                }

                void operator()(callback_type &c) const {
                    c(_a, _b);
                }
            };

            template<typename A, typename B, typename C>
            struct invoker_3 {
                A &_a;
                B &_b;
                C &_c;

                invoker_3(A &a, B &b, C &c) :
                        _a(a),
                        _b(b),
                        _c(c) {
                }

                void operator()(callback_type &c) const {
                    c(_a, _b, _c);
                }
            };


            template<typename A>
            struct invoker_1_const {
                const A &_a;

                invoker_1_const(const A &a) :
                        _a(a) {
                }

                void operator()(callback_type &c) const {
                    c(_a);
                }
            };

            template<typename A, typename B>
            struct invoker_2_const {
                const A &_a;
                const B &_b;

                invoker_2_const(const A &a, const B &b) :
                        _a(a),
                        _b(b) {
                }

                void operator()(callback_type &c) const {
                    c(_a, _b);
                }
            };

            template<typename A, typename B, typename C>
            struct invoker_3_const {
                const A &_a;
                const B &_b;
                const C &_c;

                invoker_3_const(const A &a, const B &b, const C &c) :
                        _a(a),
                        _b(b),
                        _c(c) {
                }

                void operator()(callback_type &c) const {
                    c(_a, _b, _c);
                }
            };


            template<class INVOKER>
            void _invoke(const INVOKER &invoker) {
                std::vector<receiver *> deadIds;
                if (!_safeMethods.empty()) {
                    for (method_vec_iterator it(_safeMethods.begin()), end(_safeMethods.end()); it != end; ++it) {
                        // if this slot is still active, run it, otherwise mark that it needs pruning
                        receiver *rec = (receiver *) it->first;
                        if (_registry.has(rec)) {
                            // if the receiver isn't being destroyed
                            if (!detail::receiver_is_destructing(rec)) {
                                invoker(it->second);
                            }
                        } else {
                            deadIds.push_back(rec);
                        }
                    }
                }

                //
                //	dispatch to our "unsafe" method slots
                //

                if (!_methods.empty()) {
                    for (method_vec_iterator it(_methods.begin()), end(_methods.end()); it != end; ++it) {
                        invoker(it->second);
                    }
                }

                //
                //	dispatch to free function slots
                //

                if (!_functions.empty()) {
                    for (function_vec_iterator it(_functions.begin()), end(_functions.end()); it != end; ++it) {
                        invoker(it->second);
                    }
                }

                //
                //	clean up any dead safe method slots
                //

                if (!deadIds.empty()) {
                    _safeMethods.erase(
                            std::remove_if(_safeMethods.begin(), _safeMethods.end(), same_receivers(deadIds)),
                            _safeMethods.end());
                }
            }

            template<class T, class CALLBACK>
            void _connectMethodCallback(T *obj, const CALLBACK &cb) {
                void *objId = (receiver *) (obj);

                //
                //	this object is derived from receiver, so we want to set up
                //	bidirectional connectivity
                //

                if (std::is_convertible<T *, receiver *>::value) {
                    _safeMethods.push_back(method_vec_value_type(objId, cb));

                    receiver *rec = static_cast<receiver *>(obj);
                    detail::add_registry_to_receiver(&_registry, rec);
                    _registry.add(rec);
                } else {
                    _methods.push_back(method_vec_value_type(objId, cb));
                }
            }


            method_vec _safeMethods, _methods;
            function_vec _functions;
            detail::slot_registry _registry;

        };

        namespace detail {

            inline void add_registry_to_receiver(slot_registry *registry, receiver *rec) {
                rec->_registries.insert(registry);
            }

            inline void remove_registry_from_receiver(slot_registry *registry, receiver *rec) {
                rec->_registries.erase(registry);
            }

            inline bool receiver_is_destructing(receiver *rec) {
                return rec->_destructing;
            }

        }

    }
} // core::signals

#endif /* Signals_h */
