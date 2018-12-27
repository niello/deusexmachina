using System;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace CreatorIDE
{
    public partial class Matrix44EditorCtl : UserControl
    {
        Matrix44Ref _mtx, _backup;
        bool _isUpdatingUi;
        readonly IWindowsFormsEditorService _svc;

        public Matrix44EditorCtl(IWindowsFormsEditorService svc)
        {
            InitializeComponent();
            _svc = svc;
        }

        public Matrix44Ref Matrix
        {
            get { return _mtx; }
            set
            {
                if (_mtx.IsEmpty) tcModes.SelectedTab = tpMatrix;
                _mtx = new Matrix44Ref(value);
                _backup = new Matrix44Ref(_mtx);
                RefreshSelectedPage();
            }
        }

        private void tcModes_SelectedIndexChanged(object sender, EventArgs e)
        {
            RefreshSelectedPage();
        }

        private void RefreshSelectedPage()
        {
            _isUpdatingUi = true;
            if (tcModes.SelectedTab == tpTF)
            { 
                tfSX.Value =
                    (decimal)(Math.Sqrt((double)(_mtx[0, 0] * _mtx[0, 0] + _mtx[0, 1] * _mtx[0, 1] + _mtx[0, 2] * _mtx[0, 2])));
                tfSY.Value =
                    (decimal)(Math.Sqrt((double)(_mtx[1, 0] * _mtx[1, 0] + _mtx[1, 1] * _mtx[1, 1] + _mtx[1, 2] * _mtx[1, 2])));
                tfSZ.Value =
                    (decimal)(Math.Sqrt((double)(_mtx[2, 0] * _mtx[2, 0] + _mtx[2, 1] * _mtx[2, 1] + _mtx[2, 2] * _mtx[2, 2])));

                // Convert rotation matrix to axis-angle
                float M00 = _mtx[0, 0] / (float)tfSX.Value;
                float M01 = _mtx[0, 1] / (float)tfSX.Value;
                float M02 = _mtx[0, 2] / (float)tfSX.Value;
                float M10 = _mtx[1, 0] / (float)tfSY.Value;
                float M11 = _mtx[1, 1] / (float)tfSY.Value;
                float M12 = _mtx[1, 2] / (float)tfSY.Value;
                float M20 = _mtx[2, 0] / (float)tfSZ.Value;
                float M21 = _mtx[2, 1] / (float)tfSZ.Value;
                float M22 = _mtx[2, 2] / (float)tfSZ.Value;

                double Angle = Math.Acos((M00 + M11 + M22 - 1) * 0.5f);
                double SinA = Math.Sin(Angle);
                if (Math.Abs(SinA) < 0.0001)
                {
                    tfAxisX.Value = 0;
                    tfAxisY.Value = 1;
                    tfAxisZ.Value = 0;
                    tfAngle.Value = 0;
                }
                else
                {
                    double InvDblSin = 0.5f / SinA;
                    tfAxisX.Value = (decimal)((M21 - M12) * InvDblSin);
                    tfAxisY.Value = (decimal)((M02 - M20) * InvDblSin);
                    tfAxisZ.Value = (decimal)((M10 - M01) * InvDblSin);
                    tfAngle.Value = (decimal)(Angle * 180.0 / Math.PI);
                }
                
                tfPosX.Value = (decimal)_mtx[3, 0];
                tfPosY.Value = (decimal)_mtx[3, 1];
                tfPosZ.Value = (decimal)_mtx[3, 2];
            }
            else if (tcModes.SelectedTab == tpMatrix)
            {
                m00.Value = (decimal)_mtx[0, 0];
                m01.Value = (decimal)_mtx[0, 1];
                m02.Value = (decimal)_mtx[0, 2];
                m03.Value = (decimal)_mtx[0, 3];
                m10.Value = (decimal)_mtx[1, 0];
                m11.Value = (decimal)_mtx[1, 1];
                m12.Value = (decimal)_mtx[1, 2];
                m13.Value = (decimal)_mtx[1, 3];
                m20.Value = (decimal)_mtx[2, 0];
                m21.Value = (decimal)_mtx[2, 1];
                m22.Value = (decimal)_mtx[2, 2];
                m23.Value = (decimal)_mtx[2, 3];
                m30.Value = (decimal)_mtx[3, 0];
                m31.Value = (decimal)_mtx[3, 1];
                m32.Value = (decimal)_mtx[3, 2];
                m33.Value = (decimal)_mtx[3, 3];
            }
            _isUpdatingUi = false;
        }

        private void DegreeNumUpDown_ValueChanged(object sender, EventArgs e)
        {
            if (!_isUpdatingUi)
            {
                NumericUpDown self = sender as NumericUpDown;
                self.ValueChanged -= DegreeNumUpDown_ValueChanged;
                if (self.Value >= 360) self.Value -= 360;
                else if (self.Value < 0) self.Value += 360;
                self.ValueChanged += DegreeNumUpDown_ValueChanged;
                UpdateMatrix();
            }
        }

        private void NumUpDown_ValueChanged(object sender, EventArgs e)
        {
            if (!_isUpdatingUi) UpdateMatrix();
        }

        private void UpdateMatrix()
        {
            if (tcModes.SelectedTab == tpTF)
            {
                // Rotation axis normalization
                float VX = (float)tfAxisX.Value;
                float VY = (float)tfAxisY.Value;
                float VZ = (float)tfAxisZ.Value;
                float InvLen = 1.0f / (float)Math.Sqrt(VX * VX + VY * VY + VZ * VZ);
                VX *= InvLen;
                VY *= InvLen;
                VZ *= InvLen;

                double Angle = ((double)tfAngle.Value / 180.0 * Math.PI);
                float SinA = (float)Math.Sin(Angle);
                float CosA = (float)Math.Cos(Angle);

                _mtx[0, 0] = (float)tfSX.Value * (CosA + (1.0f - CosA) * VX * VX);
                _mtx[0, 1] = (float)tfSX.Value * ((1.0f - CosA) * VX * VY - SinA * VZ);
                _mtx[0, 2] = (float)tfSX.Value * ((1.0f - CosA) * VZ * VX + SinA * VY);
                _mtx[0, 3] = 0.0f;
                _mtx[1, 0] = (float)tfSY.Value * ((1.0f - CosA) * VX * VY + SinA * VZ);
                _mtx[1, 1] = (float)tfSY.Value * (CosA + (1.0f - CosA) * VY * VY);
                _mtx[1, 2] = (float)tfSY.Value * ((1.0f - CosA) * VY * VZ - SinA * VX);
                _mtx[1, 3] = 0.0f;
                _mtx[2, 0] = (float)tfSZ.Value * ((1.0f - CosA) * VZ * VX - SinA * VY);
                _mtx[2, 1] = (float)tfSZ.Value * ((1.0f - CosA) * VY * VZ + SinA * VX);
                _mtx[2, 2] = (float)tfSZ.Value * (CosA + (1.0f - CosA) * VZ * VZ);
                _mtx[2, 3] = 0.0f;
                _mtx[3, 0] = (float)tfPosX.Value;
                _mtx[3, 1] = (float)tfPosY.Value;
                _mtx[3, 2] = (float)tfPosZ.Value;
                _mtx[3, 3] = 1.0f;
            }
            else if (tcModes.SelectedTab == tpMatrix)
            {
                _mtx[0, 0] = (float)m00.Value;
                _mtx[0, 1] = (float)m01.Value;
                _mtx[0, 2] = (float)m02.Value;
                _mtx[0, 3] = (float)m03.Value;
                _mtx[1, 0] = (float)m10.Value;
                _mtx[1, 1] = (float)m11.Value;
                _mtx[1, 2] = (float)m12.Value;
                _mtx[1, 3] = (float)m13.Value;
                _mtx[2, 0] = (float)m20.Value;
                _mtx[2, 1] = (float)m21.Value;
                _mtx[2, 2] = (float)m22.Value;
                _mtx[2, 3] = (float)m23.Value;
                _mtx[3, 0] = (float)m30.Value;
                _mtx[3, 1] = (float)m31.Value;
                _mtx[3, 2] = (float)m32.Value;
                _mtx[3, 3] = (float)m33.Value;
            }
        }

        private void bCancel_Click(object sender, EventArgs e)
        {
            _mtx = new Matrix44Ref(_backup);
            _svc.CloseDropDown();
        }

        private void bApply_Click(object sender, EventArgs e)
        {
            _svc.CloseDropDown();
        }
    }
}
