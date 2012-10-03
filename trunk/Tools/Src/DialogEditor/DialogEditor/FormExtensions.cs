using System;
using System.Windows.Forms;

namespace DialogDesigner
{
    public static class FormExtensions
    {
        public static void ShowError(this Form form, string message, params object[] args)
        {
            MessageBox.Show(form, string.Format(message, args), form.Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        public static void ShowError(this Control control, string message, params object[] args)
        {
            var c = control;
            while(c!=null && !(c is Form))
            {
                c = c.Parent;
            }

            string strMessage = string.Format(message, args);
            if (c == null)
                MessageBox.Show(c, strMessage, @"Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            else
                MessageBox.Show(c, string.Format(message, args), c.Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        public static void ShowError(this Control control, Exception ex)
        {
            ShowError(control, ex.Message);
        }
    }
}
