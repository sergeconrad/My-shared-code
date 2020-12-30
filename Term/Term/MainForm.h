#pragma once

#include <vcclr.h> 

#include "ShellConsoleControl.h"
#include "AboutForm.h"

namespace Term {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MainForm
	/// </summary>
	public ref class MainForm : public System::Windows::Forms::Form
	{
	public:
		MainForm(void)
		{
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
			
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MainForm()
		{
			if (components)
			{
				delete components;
			}
		}

	private: System::Windows::Forms::TabControl^  shellsTabControl;
	private: System::Windows::Forms::Panel^  renamePanel;
	private: System::Windows::Forms::Button^  btnCancel;
	private: System::Windows::Forms::Button^  btnDone;
	private: System::Windows::Forms::TextBox^  txtTabText;

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MainForm::typeid));
			this->shellsTabControl = (gcnew System::Windows::Forms::TabControl());
			this->renamePanel = (gcnew System::Windows::Forms::Panel());
			this->btnCancel = (gcnew System::Windows::Forms::Button());
			this->btnDone = (gcnew System::Windows::Forms::Button());
			this->txtTabText = (gcnew System::Windows::Forms::TextBox());
			this->renamePanel->SuspendLayout();
			this->SuspendLayout();
			// 
			// shellsTabControl
			// 
			this->shellsTabControl->Dock = System::Windows::Forms::DockStyle::Fill;
			this->shellsTabControl->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->shellsTabControl->Location = System::Drawing::Point(0, 0);
			this->shellsTabControl->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->shellsTabControl->Name = L"shellsTabControl";
			this->shellsTabControl->SelectedIndex = 0;
			this->shellsTabControl->Size = System::Drawing::Size(1084, 661);
			this->shellsTabControl->TabIndex = 0;
			this->shellsTabControl->SelectedIndexChanged += gcnew System::EventHandler(this, &MainForm::shellsTabControl_SelectedIndexChanged);
			this->shellsTabControl->ControlAdded += gcnew System::Windows::Forms::ControlEventHandler(this, &MainForm::shellsTabControl_OnTabPageAdded);
			this->shellsTabControl->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::shellsTabControl_MouseDown);
			// 
			// renamePanel
			// 
			this->renamePanel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->renamePanel->BackColor = System::Drawing::SystemColors::Window;
			this->renamePanel->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			this->renamePanel->Controls->Add(this->btnCancel);
			this->renamePanel->Controls->Add(this->btnDone);
			this->renamePanel->Controls->Add(this->txtTabText);
			this->renamePanel->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->renamePanel->Location = System::Drawing::Point(582, 16);
			this->renamePanel->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->renamePanel->Name = L"renamePanel";
			this->renamePanel->Size = System::Drawing::Size(467, 62);
			this->renamePanel->TabIndex = 1;
			this->renamePanel->Visible = false;
			// 
			// btnCancel
			// 
			this->btnCancel->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->btnCancel->Location = System::Drawing::Point(374, 16);
			this->btnCancel->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->btnCancel->Name = L"btnCancel";
			this->btnCancel->Size = System::Drawing::Size(77, 33);
			this->btnCancel->TabIndex = 2;
			this->btnCancel->Text = L"cancel";
			this->btnCancel->UseVisualStyleBackColor = true;
			this->btnCancel->Click += gcnew System::EventHandler(this, &MainForm::renamePanel_Cancel_Click);
			// 
			// btnDone
			// 
			this->btnDone->FlatStyle = System::Windows::Forms::FlatStyle::Flat;
			this->btnDone->Location = System::Drawing::Point(306, 16);
			this->btnDone->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->btnDone->Name = L"btnDone";
			this->btnDone->Size = System::Drawing::Size(62, 33);
			this->btnDone->TabIndex = 1;
			this->btnDone->Text = L"done";
			this->btnDone->UseVisualStyleBackColor = true;
			this->btnDone->Click += gcnew System::EventHandler(this, &MainForm::renamePanel_Done_Click);
			// 
			// txtTabText
			// 
			this->txtTabText->Location = System::Drawing::Point(17, 20);
			this->txtTabText->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->txtTabText->Name = L"txtTabText";
			this->txtTabText->Size = System::Drawing::Size(269, 22);
			this->txtTabText->TabIndex = 0;
			this->txtTabText->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::renamePanel_TabText_KeyDown);
			// 
			// MainForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(7, 17);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::SystemColors::Control;
			this->ClientSize = System::Drawing::Size(1084, 661);
			this->Controls->Add(this->renamePanel);
			this->Controls->Add(this->shellsTabControl);
			this->Font = (gcnew System::Drawing::Font(L"Segoe UI", 9.75F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->Icon = (cli::safe_cast<System::Drawing::Icon^>(resources->GetObject(L"$this.Icon")));
			this->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->Name = L"MainForm";
			this->Text = L"Windows Terminal+";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &MainForm::MainForm_FormClosing);
			this->Load += gcnew System::EventHandler(this, &MainForm::MainForm_Load);
			this->renamePanel->ResumeLayout(false);
			this->renamePanel->PerformLayout();
			this->ResumeLayout(false);

		}
#pragma endregion

	private:
		const int SYSMENU_CREATE_COMMAND_SHELL_ID = 0x1;
		const int SYSMENU_CREATE_POWERSHELL_SHELL_ID = 0x2;
		const int SYSMENU_ABOUT_ID = 0x3;

		const Keys SYSMENU_CREATE_COMMAND_SHORTCUT = (Keys::Alt | Keys::B);
		const Keys SYSMENU_CREATE_POWERSHELL_SHORTCUT = (Keys::Alt | Keys::N);

		const Point NON_EXIST_LOCATION = Point(-100, -100);
		const int WRONG_INDEX = -1;

		int rightClickedTabPageIndex = WRONG_INDEX;
		System::Drawing::Point rightClickLocation = NON_EXIST_LOCATION;

		System::Void MainForm_Load(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			//shellsTabControl->ControlAdded 
			//	+= gcnew System::Windows::Forms::ControlEventHandler(this, &Term::MainForm::shellsTabControl_OnTabPageAdded);
			// create first shell control window
			AddNewShellWindowTab(ShellConsoleControl::ShellType::Command);
			return;
		}
		System::Void MainForm_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			KillAllShellProcesses();
			return;
		}

		System::Void shellsTabControl_OnTabPageAdded(System::Object ^sender, System::Windows::Forms::ControlEventArgs ^e) {
			Debug::WriteLine(__FUNCTION__);
			// Set title and set focus on new created TabPage
			auto tabPage = dynamic_cast<TabPage^>(e->Control);
			ShellConsoleControl^ shell = dynamic_cast<ShellConsoleControl^>(tabPage->Controls[0]);
			tabPage->Text = shell->GetShellWindowTitle();
			dynamic_cast<TabControl^>(tabPage->Parent)->SelectedTab = tabPage;
			return;
		}
		System::Void shellsTabControl_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			auto tabControl = dynamic_cast<TabControl^>(sender);
			auto tabPage = dynamic_cast<TabPage^>(tabControl->SelectedTab);
			auto shell = dynamic_cast<ShellConsoleControl^>(tabPage->Controls[0]);
			shell->SetFocus();
			return;
		}
		System::Void shellConsoleControl_OnShellExited(System::Object ^sender, System::EventArgs ^e) {
			Debug::WriteLine(__FUNCTION__);
			// ignore exceptions that are raised on shutdown sometimes
			try {
				// close app on the last tab/shell
				if (shellsTabControl->Controls->Count == 1) {
					Application::Exit();
					return;
				}
				// remove tabpage
				for each(auto tab in this->shellsTabControl->Controls) {
					TabPage^ tabPage = dynamic_cast<TabPage^>(tab);
					if (sender->Equals(tabPage->Controls[0])) { // sender is 'ShellConsoleControl'
						Debug::WriteLine("TabPage FOUND.");
						RemoveTabPage(tabPage);
					}
				}
			}
			catch (Exception^ ex) {
				Debug::WriteLine(__FUNCTION__ + " Exception: " + ex->Message);
			}
			return;
		}

		// main
		void AddNewShellWindowTab(ShellConsoleControl::ShellType shellType) {
			Debug::WriteLine(__FUNCTION__);
			this->SuspendLayout();

			// create shell window control
			//auto shellControl = (gcnew Term::ShellConsoleControl());
			auto shellControl = (gcnew Term::ShellConsoleControl(shellType, this));
			shellControl->SuspendLayout();

			// initialize ShellControl
			shellControl->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
				| System::Windows::Forms::AnchorStyles::Left)
				| System::Windows::Forms::AnchorStyles::Right));
			shellControl->Location = System::Drawing::Point(0, 0);
			shellControl->Name = L"";
			shellControl->Size = System::Drawing::Size(666, 436);
			shellControl->TabIndex = 1;
			shellControl->Exited += gcnew System::EventHandler(this, &Term::MainForm::shellConsoleControl_OnShellExited);

			// create new TabPage
			auto tabPage = (gcnew System::Windows::Forms::TabPage());
			tabPage->SuspendLayout();

			// initialize TabPage
			tabPage->Location = System::Drawing::Point(4, 25);
			tabPage->Padding = System::Windows::Forms::Padding(3);
			tabPage->Size = System::Drawing::Size(658, 432);
			tabPage->UseVisualStyleBackColor = true;
			tabPage->Dock = DockStyle::Fill;

			// add ShellControl to TabPage
			tabPage->Controls->Add(shellControl);
			// add TabPage to TabControl
			this->shellsTabControl->Controls->Add(tabPage);

			tabPage->ResumeLayout();
			shellControl->ResumeLayout();

			this->ResumeLayout();
			return;
		}
		void KillAllShellProcesses() {
			Debug::WriteLine(__FUNCTION__);
			// iterate all tab pages
			for each(auto tab in this->shellsTabControl->Controls) {
				TabPage^ tabPage = dynamic_cast<TabPage^>(tab);
				ShellConsoleControl^ shellControl = dynamic_cast<ShellConsoleControl^>(tabPage->Controls[0]);
				shellControl->KillShellProcess();
			}
			return;
		}

		delegate void RemoveTabPageCallback(TabPage^ tabPage);
		void RemoveTabPage(TabPage^ tabPage) {
			Debug::WriteLine(__FUNCTION__);
			try {
				if (shellsTabControl->InvokeRequired) {
					Debug::WriteLine("INVOKE REQUIRED! SETTING CALLBACK ...");
					RemoveTabPageCallback^ rmPageCallback = gcnew RemoveTabPageCallback(this, &MainForm::RemoveTabPage);
					array<Object^>^ param = gcnew array<Object^>(1);
					param[0] = tabPage;
					try {
						this->Invoke(rmPageCallback, param);
					}
					catch (ObjectDisposedException^ ex) {
						Debug::WriteLine(__FUNCTION__ + " Exception - Inner : " + ex->Message);
					}
				}
				else {
					Debug::WriteLine("REMOVING TABPAGE");
					shellsTabControl->Controls->Remove(tabPage);
					shellsTabControl->SelectedIndex = shellsTabControl->Controls->Count - 1;
				}
			}
			catch (Exception^ ex) {
				Debug::WriteLine(__FUNCTION__ + " Exception -Outer : " + ex->Message);
			}
			return;
		}

		// menu's creation
		void ModifySystemMenu() {
			// add menu items to system menu
			HMENU sysMenu = GetSystemMenu((HWND)this->Handle.ToPointer(), false);
			AppendMenu(sysMenu, MF_SEPARATOR, NULL, NULL);
			AppendMenu(sysMenu, MF_STRING, SYSMENU_ABOUT_ID, L"&About...");
			AppendMenu(sysMenu, MF_SEPARATOR, NULL, NULL);
			AppendMenu(sysMenu, MF_STRING, SYSMENU_CREATE_COMMAND_SHELL_ID, L"New Command\tAlt+B");
			AppendMenu(sysMenu, MF_STRING, SYSMENU_CREATE_POWERSHELL_SHELL_ID, L"New Powershell\tAlt+N");
			return;
		}
		void CreateContextMenu() {
			// create
			array<ToolStripMenuItem^>^ menuItems = { 
				gcnew ToolStripMenuItem(L"Add Command Prompt", nullptr, gcnew System::EventHandler(this,&MainForm::OnAddCommandTabContextMenuClick)),
				gcnew ToolStripMenuItem(L"Add Powershell", nullptr, gcnew System::EventHandler(this,&MainForm::OnAddPowershellTabContextMenuClick)),
				gcnew ToolStripMenuItem(L"Rename", nullptr, gcnew System::EventHandler(this,&MainForm::OnRenameContextMenuClick)),
				gcnew ToolStripMenuItem(L"Close", nullptr, gcnew System::EventHandler(this,&MainForm::OnCloseContextMenuClick))
			};
			System::Windows::Forms::ContextMenuStrip^ contextMenu = gcnew System::Windows::Forms::ContextMenuStrip();
			contextMenu->Items->AddRange(menuItems);
			// install
			shellsTabControl->ContextMenuStrip = contextMenu;
			//this->ContextMenuStrip = contextMenu;
			// done.
			return;
		}
		
		// rename panel helpers
		void ShowRenamePanel(Point location, String^ text) {
			renamePanel->Location = location;;
			renamePanel->Visible = true;
			txtTabText->Text = text;
			txtTabText->Focus();
			txtTabText->SelectAll();
			return;
		}
		void HideRenamePanel() {
			renamePanel->Visible = false;
			txtTabText->Text = String::Empty;
			return;
		}
		void CloseRenamePanel() {
			HideRenamePanel();
			rightClickLocation = NON_EXIST_LOCATION;
			return;
		}

		// context menu event handlers
		System::Void OnAddCommandTabContextMenuClick(System::Object^  sender, System::EventArgs^  e) {
			AddNewShellWindowTab(ShellConsoleControl::ShellType::Command);
			return;
		}
		System::Void OnAddPowershellTabContextMenuClick(System::Object^  sender, System::EventArgs^  e) {
			AddNewShellWindowTab(ShellConsoleControl::ShellType::Powershell);
			return;
		}
		System::Void OnRenameContextMenuClick(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			if ((rightClickedTabPageIndex != WRONG_INDEX) && (rightClickedTabPageIndex < shellsTabControl->TabCount)) {
				TabPage^ tabPage = shellsTabControl->TabPages[rightClickedTabPageIndex];
				if (rightClickLocation != NON_EXIST_LOCATION) {
					ShowRenamePanel(rightClickLocation, tabPage->Text);
				}
			}
			return;
		}
		System::Void OnCloseContextMenuClick(System::Object^  sender, System::EventArgs^  e) {
			Debug::WriteLine(__FUNCTION__);
			if ((rightClickedTabPageIndex != WRONG_INDEX) && (rightClickedTabPageIndex < shellsTabControl->TabCount)) {
				if (shellsTabControl->TabCount == 1) { // the last tab
					Application::Exit();
				}
				else {
					TabPage^ tabPage = shellsTabControl->TabPages[rightClickedTabPageIndex];
					ShellConsoleControl^ shellControl = dynamic_cast<ShellConsoleControl^>(tabPage->Controls[0]);
					if (shellControl != nullptr) {
						Debug::WriteLine(__FUNCTION__ + " calls KillShellProcess()");
						shellControl->KillShellProcess();
						RemoveTabPage(tabPage);
					}
				}
			}
			return;
		}

	protected:
		// internal window core functions
		virtual void OnHandleCreated(EventArgs^ e) override {			
			Form::OnHandleCreated(e);
			ModifySystemMenu();
			CreateContextMenu();
			// done.
			return;
		}
		virtual bool ProcessCmdKey(Message% m, Keys key) override {
			bool processed = false;
			if (key == SYSMENU_CREATE_COMMAND_SHORTCUT) {
				AddNewShellWindowTab(ShellConsoleControl::ShellType::Command);
				return true;
			}
			if (key == SYSMENU_CREATE_POWERSHELL_SHORTCUT) {
				AddNewShellWindowTab(ShellConsoleControl::ShellType::Powershell);
				return true;
			}
			return Form::ProcessCmdKey(m, key);
		}
		virtual void WndProc(Message% m) override {
			if ((m.Msg == WM_SYSCOMMAND) && ((int)m.WParam == SC_MINIMIZE)) {
				// default behavior, no action needed
				Debug::WriteLine(__FUNCTION__ + " SC_MIMINIZE message ...");
			}
			else if ((m.Msg == WM_SYSCOMMAND) && ((int)m.WParam == SYSMENU_CREATE_COMMAND_SHELL_ID)) {
				AddNewShellWindowTab(ShellConsoleControl::ShellType::Command);
			}
			else if ((m.Msg == WM_SYSCOMMAND) && ((int)m.WParam == SYSMENU_CREATE_POWERSHELL_SHELL_ID)) {
				AddNewShellWindowTab(ShellConsoleControl::ShellType::Powershell);
			}
			else if ((m.Msg == WM_SYSCOMMAND) && ((int)m.WParam == SYSMENU_ABOUT_ID)) {
				ShowAboutWindow();
			}
			Form::WndProc(m);
			return;
		}	
			
	private: 
		// right click detection
		System::Void shellsTabControl_MouseDown(System::Object^  sender, System::Windows::Forms::MouseEventArgs^  e) {
			if (e->Button == System::Windows::Forms::MouseButtons::Right) {
				// find clicked tabpage by location
				for (int i = 0; i < shellsTabControl->TabCount; ++i) {
					if (shellsTabControl->GetTabRect(i).Contains(e->Location)) {
						rightClickedTabPageIndex = i;
						rightClickLocation = e->Location;
						break;
					}
				}			
			}
			return;
		}
		// rename panel event handlers
		System::Void renamePanel_Done_Click(System::Object^  sender, System::EventArgs^  e) {
			if (rightClickedTabPageIndex != WRONG_INDEX && !String::IsNullOrEmpty(txtTabText->Text)) {
				if (shellsTabControl->TabPages[rightClickedTabPageIndex]->Text != txtTabText->Text) {
					// prepare
					ShellConsoleControl^ shellControl = dynamic_cast<ShellConsoleControl^>
						(shellsTabControl->TabPages[rightClickedTabPageIndex]->Controls[0]);
					pin_ptr<const wchar_t> title = PtrToStringChars(txtTabText->Text);
					// rename
					//shellControl->SetShellWindowTitle(title);
					shellsTabControl->TabPages[rightClickedTabPageIndex]->Text = txtTabText->Text;
					shellControl->SetFocus();
					CloseRenamePanel();
				}
			}			
			return;
		}
		System::Void renamePanel_Cancel_Click(System::Object^  sender, System::EventArgs^  e) {
			CloseRenamePanel();
			return;
		}
		System::Void renamePanel_TabText_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^  e) {
			if (e->KeyCode == Keys::Enter) {
				renamePanel_Done_Click(this, nullptr);
				e->SuppressKeyPress = true;
				e->Handled = true;
			}
			else if (e->KeyCode == Keys::Escape) {
				renamePanel_Cancel_Click(this, nullptr);
				e->SuppressKeyPress = true;
				e->Handled = true;
			}
			return;
		}

		void ShowAboutWindow() {
			AboutForm^ aboutForm = gcnew AboutForm(this);
			aboutForm->ShowDialog();
			return;
		}
};
}




