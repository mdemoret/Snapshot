#pragma once

#include <functional>
#include <map>
#include <sstream>
#include "ion/base/serialize.h"

template<typename T>
class ValueNotifier
{
public:
   // A function that is called when the value changes.
   typedef std::function<void(const T & oldValue, const T & newValue)> Listener;

   struct ListenerInfo
   {
      ListenerInfo() : enabled(false) {}
      ListenerInfo(const Listener& listener_in, bool enabled_in)
         : listener(listener_in), enabled(enabled_in)
      {}
      Listener listener;
      bool enabled;
   };

   ValueNotifier(const T& value) :
      m_IsUpdating(false),
      m_Value(value)
   {}

   // Same as above, but places the setting in the passed group.
   ~ValueNotifier() {}

   std::string ToString() const { return ion::base::ValueToString(m_Value); }

   bool FromString(const std::string& str)
   {
      T value = T();
      std::istringstream in(str);
      if (StringToValue(in, &value))
      {
         SetValue(value);
         return true;
      }
      return false;
   }

   // Direct value mutators.
   const T& GetValue() const { return m_Value; }

   void SetValue(const T& value)
   {
      if (m_Value == value)
         return;

      T oldValue = m_Value;

      m_Value = value;

      NotifyListeners(oldValue, value);
   }

   operator T() const { return m_Value; }

   void operator=(const T& value)
   {
      SetValue(value);
   }

   // Equality testers.
   bool operator==(const T& value) const
   {
      return m_Value == value;
   }

   friend bool operator==(const T& value, const ValueNotifier<T>& setting)
   {
      return setting.m_Value == value;
   }

   // Adds a function that will be called when this setting's value changes. The
   // function is identified by the passed key. The same key must be used to
   // remove the listener.
   void RegisterListener(const std::string& key, const Listener& listener);
   // Enables or disables the listener identified by key, if one exists.
   void EnableListener(const std::string& key, bool enable);
   // Removes the listener identified by key, if one exists.
   void UnregisterListener(const std::string& key);

private:
   ValueNotifier() = delete; //No default constructor

   // Notify listeners that this setting has changed.
   void NotifyListeners(const T & oldValue, const T & newValue) const
   {
      //Do not update if 
      if (m_IsUpdating)
         return;

      m_IsUpdating = true;

      for (typename ListenerMap::const_iterator it = m_Listeners.begin();
           it != m_Listeners.end(); ++it)
      {
         if (it->second.enabled)
            it->second.listener(oldValue, newValue);
      }

      m_IsUpdating = false;
   }

   typedef std::map<std::string, ListenerInfo> ListenerMap;

   ListenerMap m_Listeners;

   mutable bool m_IsUpdating;
   T m_Value;
};

// Sets a Setting<T> to a new value.  The original value will be
// restored when the ScopedSettingValue is deleted.  The Setting<T>
// must outlive the ScopedSettingValue object.
template <typename T>
class ScopedValueNotifierValue
{
public:
   // Save the original setting value, and change to the new value.
   ScopedValueNotifierValue(ValueNotifier<T>* setting, const T& value)
      : setting_(setting)
   {
      if (setting_)
      {
         original_value_ = setting_->GetValue();
         setting_->SetValue(value);
      }
   }

   // Restore the original value to the Setting<T>.
   ~ScopedValueNotifierValue()
   {
      if (setting_)
      {
         setting_->SetValue(original_value_);
      }
   }

private:
   // Setting whose value is pushed/popped.
   ValueNotifier<T>* setting_;

   // This variable maintains the original value of the Setting<T>.
   // The Setting's value is restored from original_value_ when the
   // ScopedSettingValue is destroyed.
   T original_value_;
};

template<typename T>
void ValueNotifier<T>::RegisterListener(const std::string & key, const Listener & listener)
{
   m_Listeners[key] = ListenerInfo(listener, true);
}

template<typename T>
void ValueNotifier<T>::EnableListener(const std::string& key, bool enable) 
{
   typename ListenerMap::iterator it = m_Listeners.find(key);
   if (it != m_Listeners.end())
      it->second.enabled = enable;
}

template<typename T>
void ValueNotifier<T>::UnregisterListener(const std::string& key) 
{
   m_Listeners.erase(key);
}
