// Copyright 2023, Erik Hedberg. All Rights Reserved.

#pragma once

#include <type_traits>
#include "TerminalBallisticsTraits.h"



namespace TB::SimTasks
{
	template<typename ValueType>
	struct TTaskParam
	{
		typedef std::decay_t<ValueType> ValueTypeDecayed;

		TTaskParam(ValueType InValue, const bool FallbackValidity = true)
			: Value(MoveTempIfPossible(InValue))
			, FallbackValidity(FallbackValidity)
		{}

		ValueType&& GetValue()
		{
			return MoveTempIfPossible(Value);
		}

		ValueTypeDecayed GetValueIfValid()
		{
			if (IsValid())
			{
				return Value;
			}
			else
			{
				return DefaultHelper<ValueTypeDecayed>::Get(Value);
			}
		}

		ValueTypeDecayed GetDefault()
		{
			return DefaultHelper<ValueTypeDecayed>::Get(Value, true);
		}

		bool MarkIfInvalid()
		{
			if (!IsValid())
			{
				MarkAsInvalid();
				return true;
			}
			else
			{
				return false;
			}
		}

		void MarkAsInvalid()
		{
			DefaultHelper<ValueType>::MarkInvalid(Value);
		}

		bool IsValid() const
		{
			return CheckValid(Value, FallbackValidity);
		}
	private:
		ValueType Value;
		bool FallbackValidity : 1;

		template<typename TypeToCheck>
		struct HasValid
		{
		private:
			template<typename T>
			static constexpr auto Check(void*) -> typename std::is_same<decltype(std::declval<T>().IsValid()), bool>::type;

			template<typename>
			static constexpr std::false_type Check(...);

			typedef decltype(Check<TypeToCheck>(0)) type;
		public:
			static constexpr bool value = type::value;
		};

		/**
		* If "bool IsValid()" exists on T, returns the result of calling it.
		* Otherwise returns FallbackValue.
		*/
		template<typename T>
		constexpr bool CheckValid(const T& ValueToTest, const bool FallbackValue = true) const
		{
			if constexpr(HasValid<std::remove_pointer_t<T>>::value)
			{
				if constexpr (std::is_pointer_v<T>)
				{
					if (ValueToTest)
					{
						return ValueToTest->IsValid();
					}
					else
					{
						return false; // It's a nullptr, so it can't be valid.
					}
				}
				else
				{
					return ValueToTest.IsValid();
				}
			}
			else
			{
				return FallbackValue;
			}
		}

		template<typename TypeToCheck>
		struct DefaultHelper
		{
		private:
			template<typename T>
			static constexpr auto Check(void*) -> typename std::is_same<decltype(T::GetDefault()), T>::type;
			template<typename T>
			static constexpr auto CheckMember(void*) -> typename std::is_same<decltype(T::Default), T>::type;

			template<typename T>
			static constexpr auto CheckForValidMemberFlag(void*) -> decltype(std::declval<T>().bIsValid == bool(), std::true_type());
			/* Pointer version of the above */
			template<typename T>
			static constexpr auto CheckForValidMemberFlag(void*) -> decltype(std::declval<T>()->bIsValid == bool(), std::true_type());

			template<typename>
			static constexpr std::false_type Check(...);
			template<typename>
			static constexpr std::false_type CheckMember(...);
			template<typename>
			static constexpr std::false_type CheckForValidMemberFlag(...);

		public:
			static constexpr bool value = decltype(Check<TypeToCheck>(0))::value || decltype(CheckMember<TypeToCheck>(0))::value;
			static constexpr bool hasbIsValidMember = decltype(CheckForValidMemberFlag<TypeToCheck>(0))::value;

			template<typename T>
			static void MarkInvalid(T& ValueToMark)
			{
				if constexpr (hasbIsValidMember && TB::Traits::TTypeTraitsIfInvalid<T>::MarkIfInvalid)
				{
					if constexpr (std::is_pointer_v<TypeToCheck>)
					{
						if (ValueToMark)
						{
							ValueToMark->bIsValid = false;
						}
					}
					else
					{
						ValueToMark.bIsValid = false;
					}
				}
			}

			static TypeToCheck Get(TypeToCheck OriginalValue, bool bForceDefault = false)
			{
				using Traits = TB::Traits::TTypeTraitsIfInvalid<TypeToCheck>;
				if(value)
				{
					if constexpr (decltype(Check<TypeToCheck>(0))::value)
					{
						return TypeToCheck::GetDefault();
					}
					else if constexpr (decltype(CheckMember<TypeToCheck>(0))::value)
					{
						return TypeToCheck::Default;
					}
				}
				else if ((!bForceDefault && Traits::MarkIfInvalid) || !bForceDefault) // It is invalid, but we don't want to use the default.
				{
					if constexpr (decltype(CheckForValidMemberFlag<TypeToCheck>(0))::value)
					{
						if constexpr (std::is_pointer_v<TypeToCheck>)
						{
							if (OriginalValue)
							{
								TypeToCheck TempVal = OriginalValue;
								TempVal->bIsValid = false;
								return TempVal;
							}
						}
						else
						{
							TypeToCheck TempVal = OriginalValue;
							TempVal.bIsValid = false;
							return TempVal;
						}
					}
				}
				static auto Default = std::decay_t<TypeToCheck>();
				return Default;
			}
		};
	};

}
