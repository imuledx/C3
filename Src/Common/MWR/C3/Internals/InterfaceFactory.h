#pragma once

#include "Common/MWR/CppTools/Hash.h"
#include "Interface.h"

namespace MWR::C3
{
	/// Singleton class allowing registration and access to functors creating new Interface.
	class InterfaceFactory
	{
	public:
		/// Get reference to singleton object of InterfaceFactory.
		static InterfaceFactory& Instance();

		/// Callable type creating new Interface.
		template <typename T>
		using Builder = std::function<std::shared_ptr<T>(ByteView)>;

		/// Type with all information about interface.
		template <typename T>
		struct InterfaceData
		{
			Builder<T> m_Builder;
			std::string m_Name;
			std::string m_Capability;
			HashT m_ClousureConnectorHash;
			std::pair<std::chrono::milliseconds, std::chrono::milliseconds> m_StartupJitter;
		};

		/// Register Interface builder in factory.
		/// @param hash. Used as a key for registration.
		/// @param data. All informations about interface..
		template <typename T>
		bool Register(HashT hash, InterfaceData<T> data)
		{
			return GetMap<T>().emplace(hash, std::move(data)).second; // second of pair of iterator and emplace result.
		}

		/// Return information about Interface for required hash.
		/// @param hash. Used as a key for registration.
		/// @return Interface information if found or nullptr otherwise.
		template <typename T>
		InterfaceData<T> const* GetInterfaceData(HashT hash)
		{
			try
			{
				return &Find<T>(hash)->second;
			}
			catch (...)
			{
				return nullptr;
			}
		}

		/// Find iterator to object associated to interface hash.
		/// @param hash. Used as a key for registration.
		/// @throws if interface was not registered.
		/// @return iterator to internal storage.
		template<typename T, std::enable_if_t<std::is_same_v<T, AbstractChannel> or std::is_same_v<T, AbstractPeripheral> or std::is_same_v<T, AbstractConnector>, int> = 0>
		auto Find(HashT hash)
		{
			auto it = GetMap<T>().find(hash);
			if (it == GetMap<T>().end())
				throw std::runtime_error{ OBF("Requested Device was not registered: ") + std::to_string(hash) };

			return it;
		}

		/// Find iterator to object associated to interface name.
		/// @param name Interface name
		/// @return hash of the Interface name.
		template<typename T, std::enable_if_t<std::is_same_v<T, AbstractChannel> or std::is_same_v<T, AbstractPeripheral> or std::is_same_v<T, AbstractConnector>, int> = 0>
		HashT Find(std::string_view name)
		{
			for (auto iterator = GetMap<T>().begin(); iterator != GetMap<T>().end(); ++iterator)
				if (iterator->second.m_Name == name)
					return iterator.first;

			return 0;
		}

		/// Get proper map member for type T.
		template <typename T> auto& GetMap() { static_assert(false, "Template type not supported."); }
		template <> auto& GetMap<AbstractChannel>() { return m_Channels; }
		template <> auto& GetMap<AbstractPeripheral>() { return m_Peripherals; }
		template <> auto& GetMap<AbstractConnector>() { return m_Connectors; }

		/// Return json string with capability;
		std::string GetCapability();

	private:
		/// Private constructor.
		/// Only object of InterfaceFactory singleton must be accessed by Instance method.
		InterfaceFactory() = default;

		/// Separated maps for each interface type.
		std::unordered_map<HashT, InterfaceData<AbstractChannel>> m_Channels;
		std::unordered_map<HashT, InterfaceData<AbstractPeripheral>> m_Peripherals;
		std::unordered_map<HashT, InterfaceData<AbstractConnector>> m_Connectors;
	};

	/// Use typeid to get text interface name.
	/// @return std::string interface name.
	template <typename U>
	std::string GetInterfaceName()
	{
#		ifndef _MSC_VER
		static_assert(false, "Current implementation supports only MSVC.");
#		endif

		std::string_view fullName = __FUNCSIG__;
		std::string InterfaceMask = OBF("::Interfaces::");

		size_t offset;
		if ((offset = fullName.rfind(InterfaceMask)) == std::string::npos)
			throw std::logic_error{ OBF("Cannot generate interface name.") };

		offset += InterfaceMask.size();

		// Remove Channels:: / Implants::
		if ((offset = fullName.find(OBF("::"), offset)) == std::string::npos)
			throw std::logic_error{ OBF("Cannot generate interface name.") };

		auto retVal = std::string{ fullName.substr(offset + "::"sv.size()) };

		if ((offset = retVal.rfind('>')) == std::string::npos)
			throw std::logic_error{ OBF("Cannot generate interface name.") };

		retVal = retVal.substr(0, offset);
		while ((offset = retVal.find(OBF("::"))) != std::string::npos)
			retVal.erase(offset, 1);

		return retVal;
	}
}
