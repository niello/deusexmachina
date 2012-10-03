using System;
using System.Linq;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class FormCharactersProperties : Form
    {
        private readonly ICharacterContainer _container;

        private bool _allowAddCharacter = true;
        public bool AllowAddCharacter
        {
            get { return _allowAddCharacter; }
            set
            {
                if(_allowAddCharacter!=value)
                {
                    _buttonAddCharacter.Enabled = value;
                    _comboBoxCharacters.DropDownStyle = value ? ComboBoxStyle.DropDown : ComboBoxStyle.DropDownList;
                    _allowAddCharacter = value;
                }
            }
        }

        public FormCharactersProperties(ICharacterContainer container)
        {
            InitializeComponent();

            _container = container;
        }

        private void FormCharactersPropertiesLoad(object sender, EventArgs e)
        {
            _comboBoxCharacters.Items.AddRange(_container.GetCharacters().Cast<object>().ToArray());
            if (_comboBoxCharacters.Items.Count > 0)
                _comboBoxCharacters.SelectedIndex = 0;
        }

        private void ComboBoxCharactersSelectedIndexChanged(object sender, EventArgs e)
        {
            if (_comboBoxCharacters.SelectedIndex >= 0)
            {
                var item = _comboBoxCharacters.SelectedItem;
                _propertyGrid.SelectedObject = item;
            }
            else
                _propertyGrid.SelectedObject = null;
        }

        private void ButtonAddCharacterClick(object sender, EventArgs e)
        {
            var charName = _comboBoxCharacters.Text.Trim();
            if(string.IsNullOrEmpty(charName))
            {
                ShowError("Name of a character can't be an empty string.");
                return;
            }

            if(_container.ContainsCharacter(charName))
            {
                ShowError(string.Format("Character '{0}' is already exists in list of characters.", charName));
                return;
            }

            var ch = _container.AddCharacter(charName);
            _comboBoxCharacters.Items.Insert(0, ch);
            _comboBoxCharacters.SelectedIndex = 0;
        }

        private void ShowError(string message)
        {
            MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void ButtonOkClick(object sender, EventArgs e)
        {
            Close();
        }

        private void ButtonAddPropertyClick(object sender, EventArgs e)
        {
            var dlg = new FormPropertyEditor();
            if(dlg.ShowDialog(this)==DialogResult.OK)
            {
                var depth = _container.AttributeCollection.GetPropertyDepth(dlg.PropertyDescriptor.Name);
                if(depth>=0)
                {
                    ShowError(string.Format("Property '{0}' is already exists.", dlg.PropertyDescriptor.Name));
                    return;
                }

                _container.AttributeCollection.AddProperty(dlg.PropertyDescriptor);
                _propertyGrid.Refresh();
            }
        }
    }
}
