using System;
using System.Collections.Generic;

namespace CreatorIDE.Engine
{
    public class Category
    {
        private readonly List<AttrID> _attrIDs;

        public string Name { get; set; }

        public List<AttrID> AttrIDs
        {
            get { return _attrIDs; }
        }

        public Category(CideEngine engine, int categoryIdx)
        {
            if (engine == null)
                throw new ArgumentNullException("engine");

            Name = engine.GetCategoryName(categoryIdx);
            var attrCount = engine.GetCategoryInstantAttrCount(categoryIdx);
            _attrIDs = new List<AttrID>(attrCount <= 0 ? 1 : attrCount);
            for(int i=0; i<attrCount; i++)
            {
                var attrID = engine.GetAttrID(categoryIdx, i);
                _attrIDs.Add(attrID);
            }
        }
    }
}
