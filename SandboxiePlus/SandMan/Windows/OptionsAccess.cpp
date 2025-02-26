#include "stdafx.h"
#include "OptionsWindow.h"
#include "SandMan.h"
#include "SettingsWindow.h"
#include "../MiscHelpers/Common/Settings.h"
#include "../MiscHelpers/Common/Common.h"
#include "../MiscHelpers/Common/ComboInputDialog.h"
#include "../MiscHelpers/Common/SettingsWidgets.h"
#include "Helpers/WinAdmin.h"


void COptionsWindow::CreateAccess()
{
	// Resource Access
	connect(ui.chkPrivacy, SIGNAL(clicked(bool)), this, SLOT(OnAccessChanged()));
	connect(ui.chkUseSpecificity, SIGNAL(clicked(bool)), this, SLOT(OnAccessChanged()));
	connect(ui.chkCloseForBox, SIGNAL(clicked(bool)), this, SLOT(OnAccessChanged()));
	connect(ui.chkNoOpenForBox, SIGNAL(clicked(bool)), this, SLOT(OnAccessChanged()));
	//

	connect(ui.btnAddFile, SIGNAL(clicked(bool)), this, SLOT(OnAddFile()));
	QMenu* pFileBtnMenu = new QMenu(ui.btnAddFile);
	pFileBtnMenu->addAction(tr("Browse for File"), this, SLOT(OnBrowseFile()));
	pFileBtnMenu->addAction(tr("Browse for Folder"), this, SLOT(OnBrowseFolder()));
	ui.btnAddFile->setPopupMode(QToolButton::MenuButtonPopup);
	ui.btnAddFile->setMenu(pFileBtnMenu);
	connect(ui.btnAddKey, SIGNAL(clicked(bool)), this, SLOT(OnAddKey()));
	connect(ui.btnAddIPC, SIGNAL(clicked(bool)), this, SLOT(OnAddIPC()));
	connect(ui.btnAddWnd, SIGNAL(clicked(bool)), this, SLOT(OnAddWnd()));
	connect(ui.btnAddCOM, SIGNAL(clicked(bool)), this, SLOT(OnAddCOM()));
	connect(ui.chkShowAccessTmpl, SIGNAL(clicked(bool)), this, SLOT(OnShowAccessTmpl()));
	connect(ui.btnDelAccess, SIGNAL(clicked(bool)), this, SLOT(OnDelAccess()));

	//connect(ui.treeAccess, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(OnAccessItemClicked(QTreeWidgetItem*, int)));
	connect(ui.treeAccess, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(OnAccessItemDoubleClicked(QTreeWidgetItem*, int)));
	connect(ui.treeAccess, SIGNAL(itemSelectionChanged()), this, SLOT(OnAccessSelectionChanged()));
	connect(ui.treeAccess, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnAccessChanged(QTreeWidgetItem *, int)));
}

void COptionsWindow::OnAccessChanged()
{ 
	if (ui.chkPrivacy->isChecked() || ui.chkUseSpecificity->isChecked())
		theGUI->CheckCertificate();

	UpdateAccessPolicy();

	m_AccessChanged = true; 
	OnOptChanged();
}

void COptionsWindow::UpdateAccessPolicy()
{ 
	if (ui.chkPrivacy->isChecked()) {
		ui.chkUseSpecificity->setEnabled(false);
		ui.chkUseSpecificity->setChecked(true);

		ui.chkCloseForBox->setEnabled(false);
		ui.chkCloseForBox->setChecked(false);
		ui.chkNoOpenForBox->setEnabled(false);
		ui.chkNoOpenForBox->setChecked(false);
	}
	else {
		ui.chkUseSpecificity->setEnabled(true);

		ui.chkCloseForBox->setEnabled(true);
		ui.chkNoOpenForBox->setEnabled(true);
	}
}

QTreeWidgetItem* COptionsWindow::GetAccessEntry(EAccessType Type, const QString& Program, EAccessMode Mode, const QString& Path)
{
	for (int i = 0; i < ui.treeAccess->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* pItem = ui.treeAccess->topLevelItem(i);
		if (pItem->data(0, Qt::UserRole).toInt() == Type
		 && pItem->data(1, Qt::UserRole).toString().compare(Program, Qt::CaseInsensitive) == 0
		 && pItem->data(2, Qt::UserRole).toInt() == Mode
		 && pItem->data(3, Qt::UserRole).toString().compare(Path, Qt::CaseInsensitive) == 0)
			return pItem;
	}
	return NULL;
}

void COptionsWindow::SetAccessEntry(EAccessType Type, const QString& Program, EAccessMode Mode, const QString& Path)
{
	if (GetAccessEntry(Type, Program, Mode, Path) != NULL)
		return; // already set
	m_AccessChanged = true;
	OnOptChanged();
	AddAccessEntry(Type, Mode, Program, Path);
}

void COptionsWindow::DelAccessEntry(EAccessType Type, const QString& Program, EAccessMode Mode, const QString& Path)
{
	if(QTreeWidgetItem* pItem = GetAccessEntry(Type, Program, Mode, Path))
	{
		delete pItem;
		m_AccessChanged = true;
		OnOptChanged();
	}
}

QString COptionsWindow::AccessTypeToName(EAccessEntry Type)
{
	switch (Type)
	{
	case eNormalFilePath:	return "NormalFilePath";
	case eOpenFilePath:		return "OpenFilePath";
	case eOpenPipePath:		return "OpenPipePath";
	case eClosedFilePath:	return "ClosedFilePath";
	case eReadFilePath:		return "ReadFilePath";
	case eWriteFilePath:	return "WriteFilePath";

	case eNormalKeyPath:	return "NormalKeyPath";
	case eOpenKeyPath:		return "OpenKeyPath";
	case eOpenConfPath:		return "OpenConfPath";
	case eClosedKeyPath:	return "ClosedKeyPath";
	case eReadKeyPath:		return "ReadKeyPath";
	case eWriteKeyPath:		return "WriteKeyPath";

	case eNormalIpcPath:	return "NormalIpcPath";
	case eOpenIpcPath:		return "OpenIpcPath";
	case eClosedIpcPath:	return "ClosedIpcPath";
	case eReadIpcPath:		return "ReadIpcPath";

	case eOpenWinClass:		return "OpenWinClass";

	case eOpenCOM:			return "OpenClsid";
	case eClosedCOM:		return "ClosedClsid";
	case eClosedCOM_RT:		return "ClosedRT";
	}
	return "Unknown";
}

void COptionsWindow::LoadAccessList()
{
	ui.chkPrivacy->setChecked(m_pBox->GetBool("UsePrivacyMode", false));
	ui.chkUseSpecificity->setChecked(m_pBox->GetBool("UseRuleSpecificity", false));
	ui.chkCloseForBox->setChecked(m_pBox->GetBool("AlwaysCloseForBoxed", true));
	ui.chkNoOpenForBox->setChecked(m_pBox->GetBool("DontOpenForBoxed", true));

	ui.treeAccess->clear();

	for (int i = 0; i < eMaxAccessType; i++)
	{
		foreach(const QString& Value, m_pBox->GetTextList(AccessTypeToName((EAccessEntry)i), m_Template))
			ParseAndAddAccessEntry((EAccessEntry)i, Value);

		foreach(const QString& Value, m_pBox->GetTextList(AccessTypeToName((EAccessEntry)i) + "Disabled", m_Template))
			ParseAndAddAccessEntry((EAccessEntry)i, Value, true);
	}

	LoadAccessListTmpl();

	UpdateAccessPolicy();

	m_AccessChanged = false;
}

void COptionsWindow::LoadAccessListTmpl(bool bUpdate)
{
	if (ui.chkShowAccessTmpl->isChecked())
	{
		foreach(const QString& Template, m_pBox->GetTemplates())
		{
			for (int i = 0; i < eMaxAccessType; i++)
			{
				foreach(const QString& Value, m_pBox->GetTextListTmpl(AccessTypeToName((EAccessEntry)i), Template))
					ParseAndAddAccessEntry((EAccessEntry)i, Value, false, Template);
			}
		}
	}
	else if (bUpdate)
	{
		for (int i = 0; i < ui.treeAccess->topLevelItemCount(); )
		{
			QTreeWidgetItem* pItem = ui.treeAccess->topLevelItem(i);
			int Type = pItem->data(0, Qt::UserRole).toInt();
			if (Type == -1) {
				delete pItem;
				continue; // entry from template
			}
			i++;
		}
	}
}

void COptionsWindow::ParseAndAddAccessEntry(EAccessEntry EntryType, const QString& Value, bool disabled, const QString& Template)
{
	EAccessType	Type;
	EAccessMode	Mode;
	switch (EntryType)
	{
	case eNormalFilePath:	Type = eFile;	Mode = eNormal;	break;
	case eOpenFilePath:		Type = eFile;	Mode = eOpen;	break;
	case eOpenPipePath:		Type = eFile;	Mode = eOpen4All; break;
	case eClosedFilePath:	Type = eFile;	Mode = eClosed;	break;
	case eReadFilePath:		Type = eFile;	Mode = eReadOnly; break;
	case eWriteFilePath:	Type = eFile;	Mode = eBoxOnly; break;

	case eNormalKeyPath:	Type = eKey;	Mode = eNormal;	break;
	case eOpenKeyPath:		Type = eKey;	Mode = eOpen;	break;
	case eOpenConfPath:		Type = eKey;	Mode = eOpen4All;break;
	case eClosedKeyPath:	Type = eKey;	Mode = eClosed;	break;
	case eReadKeyPath:		Type = eKey;	Mode = eReadOnly; break;
	case eWriteKeyPath:		Type = eKey;	Mode = eBoxOnly; break;

	case eNormalIpcPath:	Type = eIPC;	Mode = eNormal;	break;
	case eOpenIpcPath:		Type = eIPC;	Mode = eOpen;	break;
	case eClosedIpcPath:	Type = eIPC;	Mode = eClosed;	break;
	case eReadIpcPath:		Type = eIPC;	Mode = eReadOnly; break;

	case eOpenWinClass:		Type = eWnd;	Mode = eOpen;	break;

	case eOpenCOM:			Type = eCOM;	Mode = eOpen;	break;
	case eClosedCOM:		Type = eCOM;	Mode = eClosed;	break;
	case eClosedCOM_RT:		Type = eCOM;	Mode = eClosedRT; break;

	default:				return;
	}

	//
	// Mind this special cases
	// OpenIpcPath=$:program.exe <- full access into the address space of a target process running outside the sandbox. 
	// OpenWinClass=$:program.exe <- permits to use the PostThreadMessage API to send a message directly to a thread running outside the sandbox. 
	// This form of the setting does not support wildcards.
	//

	QStringList Values = Value.split(",");
	if (Values.count() >= 2) 
		AddAccessEntry(Type, Mode, Values[0], Values[1], disabled, Template);
	else // all programs
		AddAccessEntry(Type, Mode, "", Values[0], disabled, Template);
}

QString COptionsWindow::GetAccessModeStr(EAccessMode Mode)
{
	switch (Mode)
	{
	case eNormal:		return tr("Normal");
	case eOpen:			return tr("Open");
	case eOpen4All:		return tr("Open for All");
	case eClosed:		return tr("Closed");
	case eClosedRT:		return tr("Closed RT");
	case eReadOnly:		return tr("Read Only");
	case eBoxOnly:		return tr("Box Only (Write Only)");
	}
	return tr("Unknown");
}

QString COptionsWindow::GetAccessTypeStr(EAccessType Type)
{
	switch (Type)
	{
	case eFile:			return tr("File/Folder");
	case eKey:			return tr("Registry");
	case eIPC:			return tr("IPC Path");
	case eWnd:			return tr("Wnd Class");
	case eCOM:			return tr("COM Object");
	}
	return tr("Unknown");
}

void COptionsWindow::OnBrowseFile()
{
	QString Value = QFileDialog::getOpenFileName(this, tr("Select File"), "", tr("All Files (*.*)")).replace("/", "\\");
	if (Value.isEmpty())
		return;

	AddAccessEntry(eFile, eOpen, "", Value);

	m_AccessChanged = true;
	OnOptChanged();
}

void COptionsWindow::OnBrowseFolder()
{
	QString Value = QFileDialog::getExistingDirectory(this, tr("Select Directory")).replace("/", "\\");
	if (Value.isEmpty())
		return;

	AddAccessEntry(eFile, eOpen, "", Value);

	m_AccessChanged = true;
	OnOptChanged();
}

void COptionsWindow::AddAccessEntry(EAccessType	Type, EAccessMode Mode, QString Program, const QString& Path, bool disabled, const QString& Template)
{
	QTreeWidgetItem* pItem = new QTreeWidgetItem();

	pItem->setText(0, GetAccessTypeStr(Type) + (Template.isEmpty() ? "" : " (" + Template + ")"));
	pItem->setData(0, Qt::UserRole, !Template.isEmpty() ? -1 : (int)Type);

	pItem->setData(1, Qt::UserRole, Program);
	bool bAll = Program.isEmpty();
	if (bAll)
		Program = tr("All Programs");
	bool Not = Program.left(1) == "!";
	if (Not)
		Program.remove(0, 1);
	if (Program.left(1) == "<")
		Program = tr("Group: %1").arg(Program.mid(1, Program.length() - 2));
	else if(!bAll)
		m_Programs.insert(Program);
	pItem->setText(1, (Not ? "NOT " : "") + Program);
	
	pItem->setText(2, GetAccessModeStr(Mode));
	pItem->setData(2, Qt::UserRole, (int)Mode);

	QString sPath = Path;
	if (Type == eFile || Type == eKey) {
		if (sPath.left(1) != "|" && !sPath.contains("*") && sPath.right(1) != "*")
			sPath.append("*");
	}
	pItem->setText(3, sPath);
	pItem->setData(3, Qt::UserRole, Path);

	if(Template.isEmpty())
		pItem->setCheckState(0, disabled ? Qt::Unchecked : Qt::Checked);
	ui.treeAccess->addTopLevelItem(pItem);
}

QString COptionsWindow::MakeAccessStr(EAccessType Type, EAccessMode Mode)
{
	switch (Type)
	{
	case eFile:
		switch (Mode)
		{
		case eNormal:		return "NormalFilePath";
		case eOpen:			return "OpenFilePath";
		case eOpen4All:		return "OpenPipePath";
		case eClosed:		return "ClosedFilePath";
		case eReadOnly:		return "ReadFilePath";
		case eBoxOnly:		return "WriteFilePath";
		}
		break;
	case eKey:
		switch (Mode)
		{
		case eNormal:		return "NormalKeyPath";
		case eOpen:			return "OpenKeyPath";
		case eOpen4All:		return "OpenConfPath";
		case eClosed:		return "ClosedKeyPath";
		case eReadOnly:		return "ReadKeyPath";
		case eBoxOnly:		return "WriteKeyPath";
		}
		break;
	case eIPC:
		switch (Mode)
		{
		case eNormal:		return "NormalIpcPath";
		case eOpen:			return "OpenIpcPath";
		case eClosed:		return "ClosedIpcPath";
		case eReadOnly:		return "ReadIpcPath";
		}
		break;
	case eWnd:
		switch (Mode)
		{
		case eOpen:			return "OpenWinClass";
		}
		break;
	case eCOM:
		switch (Mode)
		{
		case eOpen:			return "OpenClsid";
		case eClosed:		return "ClosedClsid";
		case eClosedRT:		return "ClosedRT";
		}
		break;
	}
	return "Unknown";
}

/*void COptionsWindow::OnAccessItemClicked(QTreeWidgetItem* pItem, int Column)
{
	if (Column != 0)
		return;

	CloseAccessEdit(pItem);
}*/

void COptionsWindow::CloseAccessEdit(bool bSave)
{
	for (int i = 0; i < ui.treeAccess->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* pItem = ui.treeAccess->topLevelItem(i);
		CloseAccessEdit(pItem, bSave);
	}
}

void COptionsWindow::CloseAccessEdit(QTreeWidgetItem* pItem, bool bSave)
{
	QWidget* pProgram = ui.treeAccess->itemWidget(pItem, 1);
	if (!pProgram)
		return;

	if (bSave)
	{
		QHBoxLayout* pLayout = (QHBoxLayout*)pProgram->layout();
		QToolButton* pNot = (QToolButton*)pLayout->itemAt(0)->widget();
		QComboBox* pCombo = (QComboBox*)pLayout->itemAt(1)->widget();

		QComboBox* pMode = (QComboBox*)ui.treeAccess->itemWidget(pItem, 2);
		QLineEdit* pPath = (QLineEdit*)ui.treeAccess->itemWidget(pItem, 3);

		QString Program = pCombo->currentText();
		int Index = pCombo->findText(Program);
		if (Index != -1)
			Program = pCombo->itemData(Index, Qt::UserRole).toString();
		if (!Program.isEmpty() && Program.left(1) != "<")
			m_Programs.insert(Program);

		if (pItem->data(0, Qt::UserRole).toInt() == eCOM && !pPath->text().isEmpty())
		{
			bool isGUID = pPath->text().length() == 38 && pPath->text().left(1) == "{" && pPath->text().right(1) == "}";
			switch (pMode->currentData().toInt())
			{
			case eOpen:
			case eClosed:
				if (!isGUID) {
					QMessageBox::critical(this, "SandboxiePlus", tr("COM objects must be specified by their GUID, like: {00000000-0000-0000-0000-000000000000}"));
					return;
				}
				break;
			case eClosedRT:
				if (isGUID) {
					QMessageBox::critical(this, "SandboxiePlus", tr("RT interfaces must be specified by their name."));
					return;
				}
				break;
			}
		}

		pItem->setText(1, (pNot->isChecked() ? "NOT " : "") + pCombo->currentText());
		pItem->setData(1, Qt::UserRole, (pNot->isChecked() ? "!" : "") + Program);
		pItem->setText(2, GetAccessModeStr((EAccessMode)pMode->currentData().toInt()));
		pItem->setData(2, Qt::UserRole, pMode->currentData());
		pItem->setText(3, pPath->text());
		pItem->setData(3, Qt::UserRole, pPath->text());

		m_AccessChanged = true;
		OnOptChanged();
	}

	ui.treeAccess->setItemWidget(pItem, 1, NULL);
	ui.treeAccess->setItemWidget(pItem, 2, NULL);
	ui.treeAccess->setItemWidget(pItem, 3, NULL);
}

QList<COptionsWindow::EAccessMode> COptionsWindow::GetAccessModes(EAccessType Type)
{
	switch (Type)
	{
	case eFile:			return QList<EAccessMode>() << eNormal << eOpen << eOpen4All << eClosed << eReadOnly << eBoxOnly;
	case eKey:			return QList<EAccessMode>() << eNormal << eOpen << eOpen4All << eClosed << eReadOnly << eBoxOnly;
	case eIPC:			return QList<EAccessMode>() << eNormal << eOpen << eClosed << eReadOnly;
	case eWnd:			return QList<EAccessMode>() << eOpen;
	case eCOM:			return QList<EAccessMode>() << eOpen << eClosed << eClosedRT;
	}
	return QList<EAccessMode>();
}

void COptionsWindow::OnAccessItemDoubleClicked(QTreeWidgetItem* pItem, int Column)
{
	//if (Column == 0)
	//	return;

	int Type = pItem->data(0, Qt::UserRole).toInt();
	if (Type == -1) {
		QMessageBox::warning(this, "SandboxiePlus", tr("Template values can not be edited."));
		return;
	}

	QString Program = pItem->data(1, Qt::UserRole).toString();

	QWidget* pProgram = new QWidget();
	pProgram->setAutoFillBackground(true);
	QHBoxLayout* pLayout = new QHBoxLayout();
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	pProgram->setLayout(pLayout);
	QToolButton* pNot = new QToolButton(pProgram);
	pNot->setText("!");
	pNot->setCheckable(true);
	if (Program.left(1) == "!"){
		pNot->setChecked(true);
		Program.remove(0, 1);
	}
	pLayout->addWidget(pNot);
	QComboBox* pCombo = new QComboBox(pProgram);
	pCombo->addItem(tr("All Programs"), "");

	foreach(const QString Group, GetCurrentGroups()){
		QString GroupName = Group.mid(1, Group.length() - 2);
		pCombo->addItem(tr("Group: %1").arg(GroupName), Group);
	}

	foreach(const QString & Name, m_Programs)
		pCombo->addItem(Name, Name);

	pCombo->setEditable(true);
	int Index = pCombo->findData(Program);
	pCombo->setCurrentIndex(Index);
	if(Index == -1)
		pCombo->setCurrentText(Program);
	pLayout->addWidget(pCombo);

	ui.treeAccess->setItemWidget(pItem, 1, pProgram);

	QComboBox* pMode = new QComboBox();
	foreach(EAccessMode Mode, GetAccessModes((EAccessType)Type))
		pMode->addItem(GetAccessModeStr(Mode), (int)Mode);
	pMode->setCurrentIndex(pMode->findData(pItem->data(2, Qt::UserRole)));
	ui.treeAccess->setItemWidget(pItem, 2, pMode);

	QLineEdit* pPath = new QLineEdit();
	pPath->setText(pItem->data(3, Qt::UserRole).toString());
	ui.treeAccess->setItemWidget(pItem, 3, pPath);
}

void COptionsWindow::OnAccessChanged(QTreeWidgetItem* pItem, int Column)
{
	if (Column != 0)
		return;

	m_AccessChanged = true;
	OnOptChanged();
}

void COptionsWindow::DeleteAccessEntry(QTreeWidgetItem* pItem)
{
	if (!pItem)
		return;

	if (pItem->data(0, Qt::UserRole).toInt() == -1) {
		QMessageBox::warning(this, "SandboxiePlus", tr("Template values can not be removed."));
		return;
	}

	delete pItem;
}

void COptionsWindow::OnDelAccess()
{
	DeleteAccessEntry(ui.treeAccess->currentItem());
	m_AccessChanged = true;
	OnOptChanged();
}

void COptionsWindow::SaveAccessList()
{
	WriteAdvancedCheck(ui.chkPrivacy, "UsePrivacyMode", "y", "");
	WriteAdvancedCheck(ui.chkUseSpecificity, "UseRuleSpecificity", "y", "");
	WriteAdvancedCheck(ui.chkCloseForBox, "AlwaysCloseForBoxed", "", "n");
	WriteAdvancedCheck(ui.chkNoOpenForBox, "DontOpenForBoxed", "", "n");

	CloseAccessEdit(true);

	QStringList Keys = QStringList() 
		<< "NormalFilePath" << "OpenFilePath" << "OpenPipePath" << "ClosedFilePath" << "ReadFilePath" << "WriteFilePath"
		<< "NormalKeyPath" << "OpenKeyPath" << "OpenConfPath" << "ClosedKeyPath" << "ReadKeyPath" << "WriteKeyPath"
		<< "NormalIpcPath"<< "OpenIpcPath" << "ClosedIpcPath" << "ReadIpcPath" << "OpenWinClass" << "OpenClsid" << "ClosedClsid" << "ClosedRT";

	QMap<QString, QList<QString>> AccessMap;
	for (int i = 0; i < ui.treeAccess->topLevelItemCount(); i++)
	{
		QTreeWidgetItem* pItem = ui.treeAccess->topLevelItem(i);
		int Type = pItem->data(0, Qt::UserRole).toInt();
		if (Type == -1)
			continue; // entry from template
		int Mode = pItem->data(2, Qt::UserRole).toInt();
		QString Program = pItem->data(1, Qt::UserRole).toString();
		QString Value = pItem->data(3, Qt::UserRole).toString();
		if (!Program.isEmpty())
			Value.prepend(Program + ",");

		QString AccessStr = MakeAccessStr((EAccessType)Type, (EAccessMode)Mode);
		if (pItem->checkState(0) == Qt::Unchecked)
			AccessStr += "Disabled";
		AccessMap[AccessStr].append(Value);
	}

	foreach(const QString & Key, Keys) {
		WriteTextList(Key, AccessMap[Key]);
		WriteTextList(Key + "Disabled", AccessMap[Key + "Disabled"]);
	}

	m_AccessChanged = false;
}
