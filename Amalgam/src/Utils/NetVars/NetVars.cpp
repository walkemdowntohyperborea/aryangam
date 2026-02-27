#include "NetVars.h"

#include "../../SDK/Definitions/Interfaces/CHLClient.h"
#include "../Hash/FNV1A.h"
#include "../../Features/Configs/Configs.h"
#include <fstream>
#include <unordered_set>

#ifdef GetProp
#undef GetProp
#endif

int CNetVars::GetOffset(RecvTable* pTable, const char* szNetVar)
{
	auto uHash = FNV1A::Hash32(szNetVar);
	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		RecvProp* pProp = pTable->GetProp(i);
		if (uHash == FNV1A::Hash32(pProp->m_pVarName))
			return pProp->GetOffset();

		if (auto pDataTable = pProp->GetDataTable())
		{
			if (auto nOffset = GetOffset(pDataTable, szNetVar))
				return nOffset + pProp->GetOffset();
		}
	}

	return 0;
}

int CNetVars::GetNetVar(const char* szClass, const char* szNetVar)
{
	auto uHash = FNV1A::Hash32(szClass);
	for (auto pCurrNode = I::Client->GetAllClasses(); pCurrNode; pCurrNode = pCurrNode->m_pNext)
	{
		if (uHash == FNV1A::Hash32(pCurrNode->m_pNetworkName))
			return GetOffset(pCurrNode->m_pRecvTable, szNetVar);
	}

	return 0;
}

RecvProp* CNetVars::GetProp(RecvTable* pTable, const char* szNetVar)
{
	auto uHash = FNV1A::Hash32(szNetVar);
	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		RecvProp* pProp = pTable->GetProp(i);
		if (uHash == FNV1A::Hash32(pProp->m_pVarName))
			return pProp;

		if (auto pDataTable = pProp->GetDataTable())
		{
			if (pProp = GetProp(pDataTable, szNetVar))
				return pProp;
		}
	}

	return nullptr;
}

RecvProp* CNetVars::GetNetProp(const char* szClass, const char* szNetVar)
{
	auto uHash = FNV1A::Hash32(szClass);
	for (auto pCurrNode = I::Client->GetAllClasses(); pCurrNode; pCurrNode = pCurrNode->m_pNext)
	{
		if (uHash == FNV1A::Hash32(pCurrNode->m_pNetworkName))
			return GetProp(pCurrNode->m_pRecvTable, szNetVar);
	}

	return nullptr;
}

static void dumpRecvTable(RecvTable* const rt, const std::string& network_name, std::ofstream* const file = nullptr, const size_t offset = 0)
{
	if (!rt)
		return;

	auto rtNameMatch = [&](std::string_view rt_name)
		{
			for (ClientClass* cc = I::Client->GetAllClasses(); cc; cc = cc->m_pNext)
			{
				if (!cc || std::string(cc->GetName()).find(std::string(rt_name).erase(0, 3)) == std::string::npos)
					continue;

				return true;
			}

			return false;
		};

	auto propTypeToStr = [&](const SendPropType type)
		{
			switch (type)
			{
			case DPT_Int:
				return "int";
			case DPT_Float:
				return "float";
			case DPT_Vector:
				return "Vector";
			case DPT_VectorXY:
				return "Vector2D";
			case DPT_String:
				return "const char*";
			default:
				return "unknown";
			}
		};

	for (int n = 0; n < rt->GetNumProps(); n++)
	{
		RecvProp* const p = rt->GetProp(n);
		if (!p || !p->GetNumElements() || p->IsInsideArray() || (!p->GetOffset() && !p->GetDataTable()))
			continue;

		if (RecvTable* const rpdt = p->GetDataTable())
		{
			std::string name = rpdt->GetName();
			if(name.find("Rules") == std::string::npos)
				if(rtNameMatch(rpdt->GetName()))
					continue;

			dumpRecvTable(rpdt, network_name, file, offset + p->GetOffset());
		}

		std::string prop_type = propTypeToStr(p->GetType());
		std::string prop_name = p->GetName();
		std::string prop_table_name{ rt->GetName() };

		if (p->GetProxyFn() == I::Client->GetStandardRecvProxies()->m_Int32ToInt16) {
			prop_type = "short";
		}

		if (p->GetProxyFn() == I::Client->GetStandardRecvProxies()->m_Int32ToInt8)
			prop_type = "byte";

		if (prop_name.starts_with("m_iRawValue32")
			|| prop_name.starts_with("m_flValue")
			|| prop_name.find('.') != std::string::npos
			|| isdigit(prop_name.front()))
			continue;

		if (prop_name.ends_with("]"))
		{
			if (prop_name.ends_with("0]"))
				prop_name.erase(prop_name.end() - 3, prop_name.end());
			else
				continue;
		}

		if (prop_name.starts_with("m_ang"))
			prop_type = "QAngle";

		if (prop_name.starts_with("m_h"))
			prop_type = "EHANDLE";

		if (prop_name.starts_with("m_b") && prop_type != "unknown")
			prop_type = "bool";

		if (prop_name.front() == '\"' && prop_name.back() == '\"') 
		{
			prop_name.erase(prop_name.begin());
			prop_name.erase(prop_name.end() - 1);
		}

		static std::unordered_map<std::string, std::unordered_set<std::string>> m_written_netvars{};
		if (file && !m_written_netvars[network_name].contains(prop_name))
			*file << std::format("\tNETVAR({}, {}, \"{}\", \"{}\");\n", prop_name, prop_type, prop_table_name, prop_name);

		m_written_netvars[network_name].insert(prop_name);
	}
}

void CNetVars::DumpTables(bool bWriteToFile)
{
	std::ofstream file;

	if (bWriteToFile)
	{
		file.open(std::filesystem::path(F::Configs.m_sConfigPath + "netvars.hpp"));
		if (!file.is_open())
			file.close();
	}

	for (ClientClass* cc = I::Client->GetAllClasses(); cc; cc = cc->m_pNext)
	{
		if (!cc || !cc->m_pRecvTable)
			continue;

		std::string sTableName = cc->GetName();
		if (sTableName.starts_with("C") && isupper(sTableName.at(1)))
			sTableName.insert(1, "_");

		if (bWriteToFile && file)
			file << std::format("class {}\n{}\npublic:\n", sTableName, "{");

		dumpRecvTable(cc->m_pRecvTable, sTableName, bWriteToFile ? &file : nullptr, 0);

		if (bWriteToFile && file)
			file << "};\n\n";
	}

	file.close();
}