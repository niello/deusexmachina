using System;
using System.Globalization;
using System.ComponentModel;
using System.Drawing.Design;
using System.Text;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace CreatorIDE
{
    [TypeConverter(typeof(Vector4Converter))]
    public struct Vector4: IEquatable<Vector4>
    {
        float _x, _y, _z, _w;

        public Vector4(float[] data)
        {
            _x = data[0];
            _y = data[1];
            _z = data[2];
            _w = data[3];
        }

        public override string ToString()
        {
            return string.Format("({0};{1};{2};{3})", _x, _y, _z, _w);
        }

        [DefaultValue(0.0f), Description("Ось X или Красный компонент цвета")]
        public float X
        {
            get { return _x; }
            set { _x = value; }
        }

        [DefaultValue(0.0f), Description("Ось Y или Зелёный компонент цвета")]
        public float Y
        {
            get { return _y; }
            set { _y = value; }
        }

        [DefaultValue(0.0f), Description("Ось Z или Синий компонент цвета")]
        public float Z
        {
            get { return _z; }
            set { _z = value; }
        }

        [DefaultValue(0.0f), Description("Ось W или Альфа-компонент (прозрачность) цвета")]
        public float W
        {
            get { return _w; }
            set { _w = value; }
        }

        public bool Equals(Vector4 other)
        {
            return other._x == _x && other._y == _y && other._z == _z && other._w == _w;
        }

        public override bool Equals(object obj)
        {
            if (!(obj is Vector4)) return false;
            return Equals((Vector4) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                int result = _x.GetHashCode();
                result = (result*397) ^ _y.GetHashCode();
                result = (result*397) ^ _z.GetHashCode();
                result = (result*397) ^ _w.GetHashCode();
                return result;
            }
        }

        public static bool operator !=(Vector4 one, Vector4 other)
        {
            return !one.Equals(other);
        }

        public static bool operator ==(Vector4 one, Vector4 other)
        {
            return one.Equals(other);
        }

        public static bool TryParse(string str, NumberFormatInfo format, out Vector4 result)
        {
            if (str == null)
                throw new ArgumentNullException("str");
            if (format == null)
                throw new ArgumentNullException("format");

            str = str.Trim('(', ')');
            var coordStrings = str.Split(';');
            if (coordStrings.Length == 4)
            {
                var data = new float[coordStrings.Length];
                int i;
                for (i = 0; i < coordStrings.Length; i++)
                {
                    float val;
                    if (!float.TryParse(coordStrings[i], NumberStyles.Float, format, out val))
                        break;
                    data[i] = val;
                }
                if (i == coordStrings.Length)
                {
                    result = new Vector4(data);
                    return true;
                }
            }

            result = default(Vector4);
            return false;
        }

        public static bool TryParse(string str, out Vector4 result)
        {
            return TryParse(str, CultureInfo.CurrentCulture.NumberFormat, out result);
        }
    }

    public class Vector4Converter : ExpandableObjectConverter
    {
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
        {
            return destinationType == typeof(Vector4) || base.CanConvertTo(context, destinationType);
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
        {
            if (destinationType == typeof(string) && value is Vector4)
                return value.ToString();
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType)
        {
            return sourceType == typeof(string) || base.CanConvertFrom(context, sourceType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value)
        {
            string str;
            if ((str = value as string) != null)
            {
                Vector4 tmp;
                if (Vector4.TryParse(str, culture.NumberFormat, out tmp))
                    return tmp;

                throw new FormatException(string.Format("Can not convert '{0}' to {1}.", str, typeof(Vector4).FullName));
            }
            return base.ConvertFrom(context, culture, value);
        }

        //???is there another way?
        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context)
        {
            return true;
        }

        //???is there another way?
        public override object CreateInstance(ITypeDescriptorContext context, System.Collections.IDictionary propertyValues)
        {
            Vector4 val = new Vector4
                              {
                                  X = (float) propertyValues["X"],
                                  Y = (float) propertyValues["Y"],
                                  Z = (float) propertyValues["Z"],
                                  W = (float) propertyValues["W"]
                              };
            //context.PropertyDescriptor.SetValue(null, Val);
            return val;
        }
    }

    [Editor(typeof(Matrix44Editor), typeof(UITypeEditor))]
    public struct Matrix44Ref: IEquatable<Matrix44Ref>
    {
        private const int MatrixSize = 4;

        float[,] _m;
        private float[,] M { get { return _m ?? (_m = new float[MatrixSize,MatrixSize]); } }

        public bool IsEmpty{get { return _m == null; }}

        public Matrix44Ref(float[] data)
        {
            if(data==null)
            {
                _m = null;
                return;
            }
            if (data.Length < MatrixSize * MatrixSize)
                throw new ArgumentException(string.Format("Length of array must be equal {0}.", MatrixSize * MatrixSize), "data");
            _m = new float[MatrixSize,MatrixSize];
            
            unsafe
            {
                fixed (float* thisPtr = _m, dataPtr = data)
                    for (int i = 0; i < MatrixSize * MatrixSize; i++)
                        thisPtr[i] = dataPtr[i];
            }
        }

        public Matrix44Ref(Matrix44Ref src)
        {
            var m2 = src._m;
            if (m2 == null)
            {
                _m = null;
                return;
            }
            _m = new float[MatrixSize, MatrixSize];
            
            unsafe
            {
                fixed (float* thisPtr = _m, datPtr = m2)
                    for (int i = 0; i < MatrixSize * MatrixSize; i++)
                        thisPtr[i] = datPtr[i];
            }
        }

        public override string ToString()
        {
            var m = M;
            var builder = new StringBuilder("{ ");
            const string delimiter = "; ";
            for (int i = 0; i < MatrixSize; i++ )
            {
                builder.Append('(');
                for (int j = 0; j < MatrixSize; j++)
                    builder.Append(m[i, j]).Append(delimiter);
                builder.Length -= delimiter.Length;
                builder.Append(") ");
            }
            builder.Append('}');
            return builder.ToString();
        }

        public float[] ToArray()
        {
            float[] result = new float[MatrixSize*MatrixSize];
            if(_m!=null)
                unsafe
                {
                    fixed (float* thisPtr = _m, rPtr = result)
                        for (int i = 0; i < MatrixSize*MatrixSize; i++)
                            rPtr[i] = thisPtr[i];
                }
            return result;
        }

        public float this[int i, int j]
        {
            get { return M[i, j]; }
            set { M[i, j] = value;  }
        }

        public static bool operator ==(Matrix44Ref one, Matrix44Ref other)
        {
            return one.Equals(other);
        }

        public static bool operator !=(Matrix44Ref one, Matrix44Ref other)
        {
            return !one.Equals(other);
        }

        public override int GetHashCode()
        {
            return _m == null ? 0 : _m.GetHashCode();
        }

        public bool Equals(Matrix44Ref other)
        {
            if (ReferenceEquals(_m, other._m)) return true;
            var m1 = M;
            var m2 = other.M;

            for (int i = 0; i < MatrixSize; i++)
                for (int j = 0; j < MatrixSize; j++)
                    if (m1[i, j] != m2[i, j]) return false;
            return true;
        }

		public override bool Equals(object obj)
		{
            if(!(obj is Matrix44Ref)) return false;
		    return Equals((Matrix44Ref) obj);
		}

        public static Matrix44Ref operator *(Matrix44Ref one, Matrix44Ref other)
        {
            var res = new Matrix44Ref();
            //Zero matrix multiplication is redundant
            if(one._m==null || other._m==null)
                return res;

            unsafe
            {
                fixed (float* newM = res.M, m1 = one.M, m2 = other.M)
                {
                    for (int i = 0; i < MatrixSize; i++)
                    {
                        int pos = MatrixSize*i;
                        float mi0 = m1[pos],
                              mi1 = m1[pos + 1],
                              mi2 = m1[pos + 2],
                              mi3 = m1[pos + 3];

                        newM[pos]     = mi0 * m2[0] + mi1 * m2[MatrixSize]     + mi2 * m2[2 * MatrixSize]     + mi3 * m2[3 * MatrixSize];
                        newM[pos + 1] = mi0 * m2[1] + mi1 * m2[MatrixSize + 1] + mi2 * m2[2 * MatrixSize + 1] + mi3 * m2[3 * MatrixSize + 1];
                        newM[pos + 2] = mi0 * m2[2] + mi1 * m2[MatrixSize + 2] + mi2 * m2[2 * MatrixSize + 2] + mi3 * m2[3 * MatrixSize + 2];
                        newM[pos + 3] = mi0 * m2[3] + mi1 * m2[MatrixSize + 3] + mi2 * m2[2 * MatrixSize + 3] + mi3 * m2[3 * MatrixSize + 3];
                    }
                }
            }

            return res;
        }
    }

    public class Matrix44Editor : UITypeEditor
    {
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.DropDown; // .Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            if (value is Matrix44Ref)
            {
                IWindowsFormsEditorService Svc =
                    (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));
                if (Svc != null)
                {
                    Matrix44EditorCtl Ctl = new Matrix44EditorCtl(Svc);
                    Ctl.Matrix = (Matrix44Ref)value;
                    Svc.DropDownControl(Ctl);
                    return Ctl.Matrix;
                }
            }
            return value;
        }
    }

    public class ResourceStringEditor : UITypeEditor
    {
        readonly string _root, _ext;

        public ResourceStringEditor(string directoryRoot, string extension)
        {
            _root = directoryRoot;
            _ext = extension;
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
        {
            return UITypeEditorEditStyle.Modal;
        }

        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
        {
            if (value is string)
            {
                //!!!set wnd position near the mouse!
                var wnd = new ResourceSelectorWnd(_root, _ext) {ResourceName = (string) value};
                if (wnd.ShowDialog() == DialogResult.OK) return wnd.ResourceName;
                return value;
            }
            return value;
        }
    }
}
