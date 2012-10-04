using System;
using System.Windows.Forms;

namespace CreatorIDE
{
    public partial class GetStringWnd : Form
    {
        string _backup;

        public string UserString
        {
            get { return tStr.Text; }
            set { tStr.Text = value; _backup = value; }
        }

        public GetStringWnd()
        {
            InitializeComponent();
            DialogResult = DialogResult.Cancel;
        }

        private void OnOkButtonClick(object sender, EventArgs e)
        {
            DialogResult = DialogResult.OK;
            Close();
        }

        private void OnCancelButtonClick(object sender, EventArgs e)
        {
            UserString = _backup;
            Close();
        }
    }
}
