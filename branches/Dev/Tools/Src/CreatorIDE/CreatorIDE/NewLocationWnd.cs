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
	public partial class NewLocationWnd : Form
	{
		public string ID { get { return tID.Text; } }
		public string LevelName { get { return tName.Text; } }
		public string NavMesh { get { return tNavMesh.Text; } }
		public float[] Center
		{
			get
			{
				float[] Data = new float[3];
				Data[0] = (float)tfPosX.Value;
				Data[1] = (float)tfPosY.Value;
				Data[2] = (float)tfPosZ.Value;
				return Data;
			}
		}
		public float[] Extents
		{
			get
			{
				float[] Data = new float[3];
				Data[0] = (float)tfSizeX.Value;
				Data[1] = (float)tfSizeY.Value;
				Data[2] = (float)tfSizeZ.Value;
				return Data;
			}
		}

		public NewLocationWnd()
		{
			InitializeComponent();
		}

		private void bCreate_Click(object sender, EventArgs e)
		{
			tID.Text = tID.Text.Trim();
			tName.Text = tName.Text.Trim();
			tNavMesh.Text = tNavMesh.Text.Trim();

			if (tID.Text.Length < 1 || tName.Text.Length < 1)
			{
				MessageBox.Show("Пожалуйста, введите корректные ID и имя");
				return;
			}

			DialogResult = DialogResult.OK;
			Close();
		}

		private void bCancel_Click(object sender, EventArgs e)
		{
			DialogResult = DialogResult.Cancel;
			Close();
		}
	}
}
