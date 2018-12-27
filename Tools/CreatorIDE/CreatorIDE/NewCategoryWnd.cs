using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace CreatorIDE
{
    public partial class NewCategoryWnd : Form
    {
        public string CatName { get { return tName.Text; } }
        public string CppClass { get { return (string)cbCppClass.SelectedItem; } }
        public string TplTable { get { return tTplTable.Text; } }
        public string InstTable { get { return tInstTable.Text; } }
        public string[] Properties
        {
            get
            {
                string[] Strings = new string[lbProperties.CheckedItems.Count];
                for (int i = 0; i < lbProperties.CheckedItems.Count; i++)
                    Strings[i] = (string)lbProperties.CheckedItems[i];
                return Strings;
            }
        }

        public NewCategoryWnd()
        {
            InitializeComponent();
        }

        private void NewCategoryWnd_Shown(object sender, EventArgs e)
        {
            lbProperties.Items.Clear();
            cbCppClass.Items.Clear();
            tTplTable.Text = "";
            tInstTable.Text = "";
            
            string Line;
            System.IO.StreamReader File = new System.IO.StreamReader("Data/Properties.txt");
            while ((Line = File.ReadLine()) != null)
            {
                Line = Line.Trim();
                if (Line.Length > 0) lbProperties.Items.Add(Line);
            }
            File.Close();

            File = new System.IO.StreamReader("Data/CppClasses.txt");
            while ((Line = File.ReadLine()) != null)
            {
                Line = Line.Trim();
                if (Line.Length > 0) cbCppClass.Items.Add(Line);
            }
            File.Close();
            cbCppClass.SelectedIndex = 0;
        }

        private void bCreate_Click(object sender, EventArgs e)
        {
            //!!!prevent entering wrong characters!
            tName.Text = tName.Text.Trim();
            tTplTable.Text = tTplTable.Text.Trim();
            tInstTable.Text = tInstTable.Text.Trim();

            if (tName.Text.Length < 1 || cbCppClass.SelectedIndex < 0)
            {
                MessageBox.Show("Пожалуйста, введите корректное имя категории и выберите класс С++ из списка");
                return;
            }
            
            //!!!not in MainWnd!
            if (MainWnd.EntityCats.ContainsKey(tName.Text))
            {
                MessageBox.Show("Категория с таким именем уже существует");
                return;
            }

            if (tTplTable.Text.Length < 1) tTplTable.Text = "Tpl" + tName.Text;
            if (tInstTable.Text.Length < 1) tInstTable.Text = "Inst" + tName.Text; ;

            //???what if no selected properties?

            DialogResult = DialogResult.OK;
            Close();
        }

        private void bCancel_Click(object sender, EventArgs e)
        {
            DialogResult = DialogResult.Cancel;
            Close();
        }

        private void tName_TextChanged(object sender, EventArgs e)
        {
            //tTplTable.Text = tTplTable.Text.Trim();
            //if (tTplTable.Text.Length == 0 || tTplTable.Text == "Tpl" + tName.Text)
        }
    }
}
