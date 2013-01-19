using System;
using System.Resources;

namespace CreatorIDE.Core
{
    public interface IResourceProvider
    {
        Guid TypeID { get; }
        ResourceManager ResourceManager { get; }
    }
}
