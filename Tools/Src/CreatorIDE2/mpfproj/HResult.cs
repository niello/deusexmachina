namespace Microsoft.VisualStudio.Project
{
    public struct HResult
    {
        public static readonly HResult
            Ok = (HResult) VSConstants.S_OK,
            False = (HResult) VSConstants.S_FALSE;

        private readonly int _hResult;

        public HResult(int hResult)
        {
            _hResult = hResult;
        }

        public static implicit operator int(HResult hResult)
        {
            return hResult._hResult;
        }

        public static explicit operator HResult(int hResult)
        {
            return new HResult(hResult);
        }

        public static bool operator ==(HResult a, HResult b)
        {
            return a.Equals(b);
        }

        public static bool operator !=(HResult a, HResult b)
        {
            return !a.Equals(b);
        }

        public bool Equals(HResult other)
        {
            return other._hResult == _hResult;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (obj.GetType() != typeof (HResult)) return false;
            return Equals((HResult) obj);
        }

        public override int GetHashCode()
        {
            return _hResult;
        }

        public override string ToString()
        {
            return string.Format("0x{0:X8}", _hResult);
        }
    }
}