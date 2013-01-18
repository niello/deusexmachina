using System.Runtime.InteropServices;

namespace ThirdPartyHelper
{
    [Guid("D8F015C0-C278-11CE-A49E-444553540000")]
    [CoClass(typeof(ShellClass))]
    [ComImport]
    public interface Shell : IShellDispatch
    {
    }
}
