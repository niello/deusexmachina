using System;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class FormPropertyEditor : Form
    {
        public DialogCharacterPropertyDescriptor PropertyDescriptor { get; private set; }

        public FormPropertyEditor()
        {
            InitializeComponent();
            CheckButtonOkEnabled();

            _comboBoxPropType.Items.AddRange(new object[] {"Integer", "String", "Boolean"});
        }

        private void CheckButtonOkEnabled()
        {
            if(string.IsNullOrEmpty(_textPropertyName.Text))
            {
                _buttonOk.Enabled = false;
                return;
            }

            if(_comboBoxPropType.SelectedIndex<0)
            {
                _buttonOk.Enabled = false;
                return;
            }

            _buttonOk.Enabled = true;
        }

        private void TextPropertyNameTextChanged(object sender, EventArgs e)
        {
            CheckButtonOkEnabled();
        }

        private void ComboBoxPropTypeSelectedValueChanged(object sender, EventArgs e)
        {
            CheckButtonOkEnabled();
        }

        private void ButtonOkClick(object sender, EventArgs e)
        {
            Type propType;
            object defVal = null;
            bool hasDefVal = _checkBoxDefaultValue.Checked;
            bool defValParsed = false;
            switch(_comboBoxPropType.SelectedItem as string)
            {
                case "Integer":
                    propType = typeof (int);
                    if(hasDefVal)
                    {
                        int intVal;
                        defValParsed = int.TryParse(_textDefaultValue.Text, out intVal);
                        defVal = intVal;
                    }
                    else 
                        defVal = default(int);
                    break;
                case "String":
                    propType = typeof (string);
                    if (hasDefVal)
                    {
                        defVal = _textDefaultValue.Text;
                        defValParsed = true;
                    }
                    break;
                case "Boolean":
                    propType = typeof (bool);
                    if (hasDefVal)
                    {
                        bool boolVal;
                        defValParsed = bool.TryParse(_textDefaultValue.Text, out boolVal);
                        defVal = boolVal;
                    }
                    else 
                        defVal = default(bool);
                    break;
                default:
                    throw new Exception("Unknown type of property.");
            }
            
            if(hasDefVal && !defValParsed)
            {
                this.ShowError(string.Format("Default value should be '{0}'.", _comboBoxPropType.SelectedItem));
                return;
            }

            PropertyDescriptor = new DialogCharacterPropertyDescriptor(_textPropertyName.Text, propType, defVal,
                                                                       new Attribute[0]);
            DialogResult = DialogResult.OK;

            Close();
        }

        private void ButtonCancelClick(object sender, System.EventArgs e)
        {
            Close();
        }
    }
}
