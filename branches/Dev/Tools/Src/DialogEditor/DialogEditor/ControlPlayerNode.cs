using System;
using System.Drawing;
using System.Windows.Forms;
using DialogLogic;

namespace DialogDesigner
{
    public partial class ControlPlayerNode:UserControl
    {
        public DialogGraphPhraseNodeBase DialogNode
        {
            get { return _bindingDialogNode.DataSource as DialogGraphPhraseNodeBase; }
            set { _bindingDialogNode.DataSource = (object) value ?? typeof (DialogGraphPhraseNodeBase); }
        }

        private Color _fieldColor;
        public Color FieldColor
        {
            get { return _fieldColor; }
            set
            {
                _fieldColor = value;
                _richTextPhrase.BackColor = _fieldColor;
            }
        }

        public event EventHandler PhraseClick;

        public ControlPlayerNode()
        {
            InitializeComponent();
            _richTextPhrase.BackColor = Color.White;
        }

        private void OnPhraseClick()
        {
            if (PhraseClick != null)
                PhraseClick(this, EventArgs.Empty);
        }

        private void RichTextPhraseMouseEnter(object sender, EventArgs e)
        {
            var rtb = (RichTextBox) sender;
            rtb.BackColor = Color.LightYellow;
        }

        private void RichTextPhraseMouseLeave(object sender, EventArgs e)
        {
            var rtb = (RichTextBox) sender;
            rtb.BackColor = FieldColor;
        }

        private void RichTextPhraseClick(object sender, EventArgs e)
        {
            OnPhraseClick();
        }
    }
}
