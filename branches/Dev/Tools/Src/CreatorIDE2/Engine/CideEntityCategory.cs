using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace CreatorIDE.Engine
{
    public class CideEntityCategory
    {
        private readonly ReadOnlyCollection<AttrID> _attrIDs;
        private readonly string _uid;

        public string UID { get { return _uid; } }

        public ReadOnlyCollection<AttrID> AttrIDs { get { return _attrIDs; } }

        internal CideEntityCategory(CideEngine engine, int categoryIdx)
        {
            if (engine == null)
                throw new ArgumentNullException("engine");

            _uid = engine.GetCategoryName(categoryIdx);
            var attrCount = engine.GetCategoryInstantAttrCount(categoryIdx);
            if (attrCount < 0)
                attrCount = 0;
            var attrIDs = new List<AttrID>(attrCount);
            for(int i=0; i<attrCount; i++)
            {
                var attrID = engine.GetAttrID(categoryIdx, i);
                attrIDs.Add(attrID);
            }

            _attrIDs = new ReadOnlyCollection<AttrID>(attrIDs);
        }
    }
}
