// This file is part of the QuantumGate project. For copyright and
// licensing information refer to the license file(s) in the project root.

#include "pch.h"
#include "TestApp.h"
#include "CIPReputationsDlg.h"
#include "Common\Util.h"

using namespace QuantumGate::Implementation;

IMPLEMENT_DYNAMIC(CIPReputationsDlg, CDialogBase)

CIPReputationsDlg::CIPReputationsDlg(CWnd* pParent) : CDialogBase(IDD_IPREPUTATIONS_DIALOG, pParent)
{}

CIPReputationsDlg::~CIPReputationsDlg()
{}

void CIPReputationsDlg::SetAccessManager(QuantumGate::Access::Manager * am) noexcept
{
	m_AccessManager = am;
}

void CIPReputationsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogBase::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CIPReputationsDlg, CDialogBase)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_IPREPUTATIONS_LIST, &CIPReputationsDlg::OnLvnItemChangedIPReputationsList)
	ON_EN_CHANGE(IDC_IP, &CIPReputationsDlg::OnEnChangeIP)
	ON_EN_CHANGE(IDC_REPUTATION, &CIPReputationsDlg::OnEnChangeReputation)
	ON_BN_CLICKED(IDC_SET_REPUTATION, &CIPReputationsDlg::OnBnClickedSetReputation)
	ON_BN_CLICKED(IDC_RESET_ALL, &CIPReputationsDlg::OnBnClickedResetAll)
	ON_BN_CLICKED(IDC_RESET_SELECTED, &CIPReputationsDlg::OnBnClickedResetSelected)
	ON_BN_CLICKED(IDC_REFRESH, &CIPReputationsDlg::OnBnClickedRefresh)
END_MESSAGE_MAP()

BOOL CIPReputationsDlg::OnInitDialog()
{
	CDialogBase::OnInitDialog();

	// Init reputation list
	auto irctrl = (CListCtrl*)GetDlgItem(IDC_IPREPUTATIONS_LIST);
	irctrl->SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);
	irctrl->InsertColumn(0, _T("IP"), LVCFMT_LEFT, GetApp()->GetScaledWidth(125));
	irctrl->InsertColumn(1, _T("Score"), LVCFMT_LEFT, GetApp()->GetScaledWidth(75));
	irctrl->InsertColumn(2, _T("Last Update Time"), LVCFMT_LEFT, GetApp()->GetScaledWidth(125));

	UpdateIPReputationList();
	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CIPReputationsDlg::UpdateIPReputationList() noexcept
{
	auto flctrl = (CListCtrl*)GetDlgItem(IDC_IPREPUTATIONS_LIST);
	flctrl->DeleteAllItems();

	if (const auto result = m_AccessManager->GetAllIPReputations(); result.Succeeded())
	{
		for (const auto& rep : *result)
		{
			const auto pos = flctrl->InsertItem(0, rep.Address.GetString().c_str());
			if (pos != -1)
			{
				flctrl->SetItemText(pos, 1, Util::FormatString(L"%d", rep.Score).c_str());

				tm time_tm{ 0 };
				std::array<WChar, 100> timestr{ 0 };

				if (localtime_s(&time_tm, &*rep.LastUpdateTime) == 0)
				{
					if (std::wcsftime(timestr.data(), timestr.size(), L"%d/%m/%Y %H:%M:%S", &time_tm) != 0)
					{
						flctrl->SetItemText(pos, 2, timestr.data());
					}
				}
			}
		}
	}
}

void CIPReputationsDlg::OnLvnItemChangedIPReputationsList(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto irctrl = (CListCtrl*)GetDlgItem(IDC_IPREPUTATIONS_LIST);
	if (irctrl->GetSelectedCount() > 0)
	{
		auto position = irctrl->GetFirstSelectedItemPosition();
		const auto pos = irctrl->GetNextSelectedItem(position);
		const auto ip = irctrl->GetItemText(pos, 0);
		const auto rep = irctrl->GetItemText(pos, 1);

		SetValue(IDC_IP, ip);
		SetValue(IDC_REPUTATION, rep);
	}

	UpdateControls();

	*pResult = 0;
}

void CIPReputationsDlg::UpdateControls() noexcept
{
	const auto ip = GetTextValue(IDC_IP);
	const auto rep = GetTextValue(IDC_REPUTATION);

	if (ip.GetLength() > 0 && rep.GetLength() > 0)
	{
		GetDlgItem(IDC_SET_REPUTATION)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_SET_REPUTATION)->EnableWindow(false);
	}

	const auto irctrl = (CListCtrl*)GetDlgItem(IDC_IPREPUTATIONS_LIST);
	if (irctrl->GetSelectedCount() > 0)
	{
		GetDlgItem(IDC_RESET_SELECTED)->EnableWindow(true);
	}
	else
	{
		GetDlgItem(IDC_RESET_SELECTED)->EnableWindow(false);
	}
}

void CIPReputationsDlg::OnEnChangeIP()
{
	UpdateControls();
}


void CIPReputationsDlg::OnEnChangeReputation()
{
	UpdateControls();
}

void CIPReputationsDlg::OnBnClickedSetReputation()
{
	const auto ip = GetTextValue(IDC_IP);
	
	Access::IPReputation iprep;
	iprep.Score = static_cast<Int16>(GetInt64Value(IDC_REPUTATION));

	if (IPAddress::TryParse(ip.GetString(), iprep.Address))
	{
		if (m_AccessManager->SetIPReputation(iprep).Succeeded())
		{
			UpdateIPReputationList();
			UpdateControls();
		}
		else AfxMessageBox(L"Failed to set IP reputation!", MB_ICONERROR);
	}
	else AfxMessageBox(L"Invalid IP address specified!", MB_ICONERROR);
}

void CIPReputationsDlg::OnBnClickedResetAll()
{
	m_AccessManager->ResetAllIPReputations();
	UpdateIPReputationList();
	UpdateControls();
}

void CIPReputationsDlg::OnBnClickedResetSelected()
{
	auto irctrl = (CListCtrl*)GetDlgItem(IDC_IPREPUTATIONS_LIST);
	if (irctrl->GetSelectedCount() > 0)
	{
		auto position = irctrl->GetFirstSelectedItemPosition();
		const auto pos = irctrl->GetNextSelectedItem(position);
		const auto ip = irctrl->GetItemText(pos, 0);
		
		if (m_AccessManager->ResetIPReputation(ip.GetString()).Succeeded())
		{
			UpdateIPReputationList();
			UpdateControls();
		}
		else AfxMessageBox(L"Failed to reset IP reputation!", MB_ICONERROR);
	}
}

void CIPReputationsDlg::OnBnClickedRefresh()
{
	UpdateIPReputationList();
	UpdateControls();
}
