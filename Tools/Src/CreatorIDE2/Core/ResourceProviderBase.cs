using System;
using System.Resources;

namespace CreatorIDE.Core
{
    public abstract class ResourceProviderBase
    {
        private readonly Guid _typeID;
        private readonly ResourceManager _resourceManager;

        public Guid TypeID { get { return _typeID; } }
        public ResourceManager ResourceManager { get { return _resourceManager; } }

        protected ResourceProviderBase(Guid typeID, ResourceManager resourceManager)
        {
            if (resourceManager == null)
                throw new ArgumentNullException("resourceManager");

            _typeID = typeID;
            _resourceManager = resourceManager;
        }
    }
}
