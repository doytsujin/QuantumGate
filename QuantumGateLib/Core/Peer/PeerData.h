// This file is part of the QuantumGate project. For copyright and
// licensing information refer to the license file(s) in the project root.

#pragma once

namespace QuantumGate::Implementation::Core::Peer
{
	// States are in chronological order; Disconnected should be last
	enum class Status
	{
		Unknown, Initialized, Connecting, Accepted, Connected, MetaExchange,
		PrimaryKeyExchange, SecondaryKeyExchange, Authentication,
		SessionInit, Ready, Disconnected
	};

	struct Data final
	{
		PeerLUID LUID{ 0 };
		Status Status{ Status::Unknown };

		PeerConnectionType Type{ PeerConnectionType::Unknown };

		bool IsRelayed{ false };
		bool IsAuthenticated{ false };
		bool IsUsingGlobalSharedSecret{ false };

		PeerUUID PeerUUID;

		Size ExtendersBytesReceived{ 0 };
		Size ExtendersBytesSent{ 0 };

		std::pair<UInt8, UInt8> LocalProtocolVersion{ ProtocolVersion::Major, ProtocolVersion::Minor };
		std::pair<UInt8, UInt8> PeerProtocolVersion{ 0, 0 };

		UInt64 LocalSessionID{ 0 };
		UInt64 PeerSessionID{ 0 };

		struct
		{
			SteadyTime ConnectedSteadyTime;

			Size BytesReceived{ 0 };
			Size BytesSent{ 0 };

			IPEndpoint LocalEndpoint;
			IPEndpoint PeerEndpoint;

			ExtenderUUIDs PeerExtenderUUIDs;
		} Cached;

		Result<PeerLUID> MatchQuery(const PeerQueryParameters& params) const noexcept
		{
			// Only if peer status is ready (handshake succeeded, etc.)
			if (Status == Status::Ready)
			{
				if ((params.Authentication == PeerQueryParameters::AuthenticationOption::Authenticated && !IsAuthenticated) ||
					(params.Authentication == PeerQueryParameters::AuthenticationOption::NotAuthenticated && IsAuthenticated))
				{
					return ResultCode::Failed;
				}

				if ((params.Relays == PeerQueryParameters::RelayOption::Relayed && !IsRelayed) ||
					(params.Relays == PeerQueryParameters::RelayOption::NotRelayed && IsRelayed))
				{
					return ResultCode::Failed;
				}

				if ((params.Connections == PeerQueryParameters::ConnectionOption::Outbound &&
					 Type == PeerConnectionType::Inbound) ||
					 (params.Connections == PeerQueryParameters::ConnectionOption::Inbound &&
					  Type == PeerConnectionType::Outbound))
				{
					return ResultCode::Failed;
				}

				if (!params.Extenders.UUIDs.empty())
				{
					auto has_one_ext = false;
					for (const auto& extuuid : params.Extenders.UUIDs)
					{
						if (Cached.PeerExtenderUUIDs.HasExtender(extuuid))
						{
							has_one_ext = true;

							if (params.Extenders.Include ==
								PeerQueryParameters::Extenders::IncludeOption::OneOf)
							{
								break;
							}
							else if (params.Extenders.Include ==
									 PeerQueryParameters::Extenders::IncludeOption::NoneOf)
							{
								return ResultCode::Failed;
							}
						}
						else
						{
							if (params.Extenders.Include ==
								PeerQueryParameters::Extenders::IncludeOption::AllOf)
							{
								return ResultCode::Failed;
							}
						}
					}

					if ((params.Extenders.Include ==
						 PeerQueryParameters::Extenders::IncludeOption::OneOf) && !has_one_ext)
					{
						return ResultCode::Failed;
					}
				}

				return { LUID };
			}

			return ResultCode::Failed;
		}
	};

	using Data_ThS = Concurrency::ThreadSafe<Data, Concurrency::SharedSpinMutex>;
}