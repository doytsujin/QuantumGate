// This file is part of the QuantumGate project. For copyright and
// licensing information refer to the license file(s) in the project root.

#include "pch.h"
#include "TestApp.h"
#include "CIPSubnetLimitsDlg.h"

#include "Common\Util.h"

using namespace QuantumGate::Implementation;

IMPLEMENT_DYNAMIC(CIPSubnetLimitsDlg, CDialogBase)

CIPSubnetLimitsDlg::CIPSubnetLimitsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogBase(IDD_IPSUBNET_LIMITS_DIALOG, pParent)
{}

CIPSubnetLimitsDlg::~CIPSubnetLimitsDlg()
{}

void CIPSubnetLimitsDlg::SetAccessManager(QuantumGate::Access::Manager* am) noexcept
{
	m_AccessManager = am;
}

void CIPSubnetLimitsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CIPSubnetLimitsDlg, CDialogBase)
	ON_BN_CLICKED(IDC_ADDLIMIT, &CIPSubnetLimitsDlg::OnBnClickedAddLimit)
	ON_CBN_SELCHANGE(IDC_TYPECOMBO, &CIPSubnetLimitsDlg::OnCbnSelChangeTypeCombo)
	ON_EN_CHANGE(IDC_CIDR, &CIPSubnetLimitsDlg::OnEnChangeCIDR)
	ON_EN_CHANGE(IDC_MAX_CONNECTIONS, &CIPSubnetLimitsDlg::OnEnChangeMaxConnections)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IPSUBNETLIMIT_LIST, &CIPSubnetLimitsDlg::OnLvnItemChangedIPSubnetLimitList)
	ON_BN_CLICKED(IDC_REMOVELIMIT, &CIPSubnetLimitsDlg::OnBnClickedRemoveLimit)
END_MESSAGE_MAP()

BOOL CIPSubnetLimitsDlg::OnInitDialog()
{
	CDialogBase::OnInitDialog();

	// Init filter type combo
	auto tcombo = (CComboBox*)GetDlgItem(IDC_TYPECOMBO);
	auto pos = tcombo->AddString(L"IPv4");
	tcombo->SetItemData(pos, static_cast<DWORD_PTR>(QuantumGate::IPAddress::Family::IPv4));
	pos = tcombo->AddString(L"IPv6");
	tcombo->SetItemData(pos, static_cast<DWORD_PTR>(QuantumGate::IPAddress::Family::IPv6));

	// Init filter list
	auto slctrl = (CListCtrl*)GetDlgItem(IDC_IPSUBNETLIMIT_LIST);
	slctrl->SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	slctrl->InsertColumn(0, _T("IP Address Family"), LVCFMT_LEFT, GetApp()->GetScaledWidth(125));
	slctrl->InsertColumn(1, _T("CIDR Leading Bits"), LVCFMT_LEFT, GetApp()->GetScaledWidth(125));
	slctrl->InsertColumn(2, _T("Max. Connections"), LVCFMT_LEFT, GetApp()->GetScaledWidth(125));

	UpdateIPSubnetLimitList();
	UpdateControls();

	return TRUE;
}

void CIPSubnetLimitsDlg::UpdateIPSubnetLimitList() noexcept
{
	auto slctrl = (CListCtrl*)GetDlgItem(IDC_IPSUBNETLIMIT_LIST);
	slctrl->DeleteAllItems();

	if (const auto result = m_AccessManager->GetAllIPSubnetLimits(); result.Succeeded())
	{
		for (auto& limit : *result)
		{
			CString type = L"IPv4";
			if (limit.AddressFamily == QuantumGate::IPAddress::Family::IPv6) type = L"IPv6";

			const auto pos = slctrl->InsertItem(0, type);
			if (pos != -1)
			{
				slctrl->SetItemText(pos, 1, limit.CIDRLeadingBits.c_str());
				slctrl->SetItemText(pos, 2, Util::FormatString(L"%zu", limit.MaximumConnections).c_str());
			}
		}
	}
}

void CIPSubnetLimitsDlg::UpdateControls() noexcept
{
	const auto cidr = GetTextValue(IDC_CIDR);
	const auto num = GetTextValue(IDC_MAX_CONNECTIONS);
	const auto sel = ((CComboBox*)GetDlgItem(IDC_TYPECOMBO))->GetCurSel();

	if (cidr.GetLength() > 0 && num.GetLength() > 0 && sel != -1)
	{
		GetDlgItem(IDC_ADDLIMIT)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_ADDLIMIT)->EnableWindow(false);
	}

	const auto slctrl = (CListCtrl*)GetDlgItem(IDC_IPSUBNETLIMIT_LIST);
	if (slctrl->GetSelectedCount() > 0)
	{
		GetDlgItem(IDC_REMOVELIMIT)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_REMOVELIMIT)->EnableWindow(false);
	}
}

void CIPSubnetLimitsDlg::OnBnClickedAddLimit()
{
	const auto sel = ((CComboBox*)GetDlgItem(IDC_TYPECOMBO))->GetCurSel();
	const auto type = static_cast<QuantumGate::IPAddress::Family>(((CComboBox*)GetDlgItem(IDC_TYPECOMBO))->GetItemData(sel));
	const auto cidr = GetTextValue(IDC_CIDR);
	const auto num = GetSizeValue(IDC_MAX_CONNECTIONS);

	const auto result = m_AccessManager->AddIPSubnetLimit(type, cidr.GetString(), num);
	if (result.Failed())
	{
		AfxMessageBox(L"Couldn't add the IP subnet limit; check the CIDR and try again.",
					  MB_ICONERROR);
	}
	else
	{
		((CComboBox*)GetDlgItem(IDC_TYPECOMBO))->SetCurSel(-1);
		SetValue(IDC_CIDR, L"");
		SetValue(IDC_MAX_CONNECTIONS, L"");

		UpdateIPSubnetLimitList();
	}
}

void CIPSubnetLimitsDlg::OnCbnSelChangeTypeCombo()
{
	UpdateControls();
}

void CIPSubnetLimitsDlg::OnEnChangeCIDR()
{
	UpdateControls();
}

void CIPSubnetLimitsDlg::OnEnChangeMaxConnections()
{
	UpdateControls();
}

void CIPSubnetLimitsDlg::OnLvnItemChangedIPSubnetLimitList(NMHDR* pNMHDR, LRESULT* pResult)
{
	UpdateControls();

	*pResult = 0;
}

void CIPSubnetLimitsDlg::OnBnClickedRemoveLimit()
{
	const auto slctrl = (CListCtrl*)GetDlgItem(IDC_IPSUBNETLIMIT_LIST);
	if (slctrl->GetSelectedCount() > 0)
	{
		auto position = slctrl->GetFirstSelectedItemPosition();
		const auto pos = slctrl->GetNextSelectedItem(position);
		const auto type = slctrl->GetItemText(pos, 0);
		const auto cidr = slctrl->GetItemText(pos, 1);

		auto ftype = QuantumGate::IPAddress::Family::IPv4;
		if (type == L"IPv6") ftype = QuantumGate::IPAddress::Family::IPv6;

		if (m_AccessManager->RemoveIPSubnetLimit(ftype, cidr.GetString()) != QuantumGate::ResultCode::Succeeded)
		{
			AfxMessageBox(L"Couldn't remove the IP subnet limit.", MB_ICONERROR);
		}
		else
		{
			UpdateIPSubnetLimitList();
			UpdateControls();
		}
	}
}
