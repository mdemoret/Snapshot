#pragma once

namespace Snapshot {namespace Util{

//template<typename F>
//struct FinalAction
//{
//   explicit FinalAction(F f) : clean_{f} {}
//
//   ~FinalAction()
//   {
//      if (enabled_)
//         clean_();
//   }
//
//   void disable() { enabled_ = false; };
//private:
//   F clean_;
//   bool enabled_{true};
//};
//
//template<typename F>
//FinalAction<F> finally(F f)
//{
//   return FinalAction<F>(f);
//}

class finally
{
   std::function<void(void)> functor;
public:
   explicit finally(const std::function<void(void)> &functor) : functor(functor) {}
   ~finally()
   {
      functor();
   }
};

}}
