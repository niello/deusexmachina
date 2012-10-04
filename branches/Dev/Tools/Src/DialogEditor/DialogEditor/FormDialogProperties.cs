using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class FormDialogProperties : Form
    {
        private readonly DialogObject _dialogObject;
        private readonly DialogInfo _dialog;
        private readonly DialogObjectManager _manager;

        public FormDialogProperties(DialogObjectManager manager, DialogObject dialog)
        {
            InitializeComponent();

            _dialogObject = dialog;
            _dialog = _dialogObject.Dialog;
            _manager = manager;

            _comboCharacters.Items.AddRange(_dialog.Dialogs.Characters.Keys.ToArray());

            foreach(var character in _dialog.DialogCharacters)
            {
                var item = _listCharacters.Items.Add(new ListViewItem(character.Name));
                item.Tag = new DialogCharacter(_dialog) {IsPlayer = character.IsPlayer, Name = character.Name};
                item.Checked = character.IsPlayer;
            }

            string name = _dialogObject.GetName();

            _textDialogName.Text = name;
            
            string scriptFileName;
            try
            {
                if(_dialog.ScriptFile!=null)
                    scriptFileName = _dialog.ScriptFile;
                else
                {
                    var filePath = Path.Combine(manager.RootDirectory, _dialogObject.RelativePath + ".lua");
                    if (File.Exists(filePath))
                        scriptFileName = name + ".lua";
                    else
                        scriptFileName = string.Empty;
                }
            }
            catch
            {
                scriptFileName = string.Empty;
            }

            _textScriptFile.Text = scriptFileName;
        }

        private void ButtonOkClick(object sender, EventArgs e)
        {
            SyncCharacters();
            _dialog.Name = _textDialogName.Text;

            _dialog.ScriptFile = string.IsNullOrEmpty(_textScriptFile.Text) ? null : _textScriptFile.Text;

            DialogResult = DialogResult.OK;
            Close();
        }

        private void SyncCharacters()
        {
            var newChDict =
                (from ListViewItem item in _listCharacters.Items select (DialogCharacter) item.Tag).ToDictionary(
                    c => c.Name);

            var listToRemove = new List<DialogCharacter>();

            foreach(var ch in _dialog.DialogCharacters)
            {
                DialogCharacter newCh;
                if(newChDict.TryGetValue(ch.Name,out newCh))
                {
                    if (!ReferenceEquals(ch, newCh))
                        ch.IsPlayer = newCh.IsPlayer;
                    newChDict.Remove(ch.Name);
                }
                else
                {
                    listToRemove.Add(ch);
                }
            }

            foreach (var chToRemove in listToRemove)
                _dialog.DialogCharacters.Remove(chToRemove);

            foreach(var newCh in newChDict.Values)
            {
                _dialog.DialogCharacters.Add(newCh);
                if (!_dialog.Dialogs.Characters.ContainsKey(newCh.Name))
                    _dialog.Dialogs.AddCharacter(newCh.Name);
            }
        }

        private void ButtonCancelClick(object sender, EventArgs e)
        {
            Close();
        }

        private void ButtonAddClick(object sender, EventArgs e)
        {
            var text = _comboCharacters.Text;
            if (text != null)
                text = text.Trim();

            if(!string.IsNullOrEmpty(text))
            {
                if(!_listCharacters.Items.Cast<ListViewItem>().Any(c=>((DialogCharacter)c.Tag).Name==text))
                {
                    var ch = new DialogCharacter(_dialog) {Name = text};

                    AddCharacter(ch);
                    
                    if(_comboCharacters.SelectedIndex<0)
                        _comboCharacters.Items.Add(text);
                }
            }
        }

        private void AddCharacter(DialogCharacter ch)
        {
            var item = _listCharacters.Items.Add(new ListViewItem(ch.Name));
            item.Tag = ch;
            item.Checked = ch.IsPlayer;
        }

        private void CmCharacterItemDeleteClick(object sender, EventArgs e)
        {
            if (_listCharacters.SelectedItems.Count <= 0) return;
            var item = _listCharacters.SelectedItems[0];
            var ch = (DialogCharacter) item.Tag;
            
            if(_dialog.IsActiveCharacter(ch))
            {
                MessageBox.Show(this,
                                string.Format(
                                    "There are phrases associated with the character '{0}' in the dialog. Unable to remove this character.",
                                    ch.Name), Text, MessageBoxButtons.OK);
                return;
            }

            _listCharacters.Items.Remove(item);
        }

        private void ListCharactersMouseClick(object sender, MouseEventArgs e)
        {
            var item = _listCharacters.GetItemAt(e.X, e.Y);

            _cmCharacterItemDelete.Enabled = item != null;
            _listCharacters.SelectedIndices.Clear();

            if(item!=null)
            {
                _listCharacters.SelectedIndices.Add(_listCharacters.Items.IndexOf(item));
            }
        }

        private void ButtonScriptBrowseClick(object sender, EventArgs e)
        {
            var ofd = new OpenFileDialog {Filter = @"Lua scripts (*.lua)|*.lua", CheckFileExists = false};

            string path;
            int idx = _dialogObject.RelativePath.LastIndexOf('\\');
            if (idx <= 0)
                path = _manager.RootDirectory;
            else
                path = Path.Combine(_manager.RootDirectory, _dialogObject.RelativePath.Substring(0, idx));

            ofd.InitialDirectory = path;
            ofd.FileName = _dialogObject.GetName() + ".lua";

            if(ofd.ShowDialog(this)==DialogResult.OK)
            {
                if (!File.Exists(ofd.FileName))
                    using (File.CreateText(ofd.FileName)) {}
                _textScriptFile.Text = _dialogObject.GetRelativePath(_manager, ofd.FileName);
            }
        }
    }
}
