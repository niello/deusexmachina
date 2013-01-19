using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;

namespace CreatorIDE.Engine
{
    internal class CideEntityCache
    {
        private class EntityCategory
        {
            private readonly CideEntityCategory _category;
            
            public CideEntityCategory Category { get { return _category; } }
            public CideEntity Entity { get; set; }

            public EntityCategory(CideEngine engine, int idx)
            {
                _category = new CideEntityCategory(engine, idx);
            }
        }

        private Dictionary<string, EntityCategory> _categories;
        private Dictionary<string, CideEntity> _entities;

        public CideEntity GetCategory(CideEngine engine, string uid)
        {
            Debug.Assert(engine != null);
            if (string.IsNullOrEmpty(uid))
                return null;

            EntityCategory cat;
            if (!LoadCategories(engine).TryGetValue(uid, out cat))
                return null;

            if (cat.Entity != null)
                return cat.Entity;

            cat.Entity = new CideEntity(engine, cat.Category);
            cat.Entity.EntityRenaming += OnCategoryRenaming;
            cat.Entity.PropertyChanged += OnCategoryPropertyChanged;

            return cat.Entity;
        }

        private void OnCategoryPropertyChanged(object sender, PropertyChangedEventArgs e)
        {
            
        }

        private void OnCategoryRenaming(object sender, PropertyChangingCancelEventArgs<string> e)
        {
            var category = (CideEntity) sender;
            Debug.Assert(_categories != null && _categories.ContainsKey(category.GuidPropValue));

            var newVal = e.NewValue;
            if (!string.IsNullOrEmpty(newVal))
                newVal = newVal.Trim();

            if (string.IsNullOrEmpty(newVal))
                throw new ArgumentException(SR.GetString(SR.EmptyCategoryNameDisallowed));

            if (newVal == e.OldValue)
            {
                e.Cancel = true;
                return;
            }

            if (_categories.ContainsKey(newVal))
                throw new DuplicateNameException(SR.GetFormatString(SR.CategoryWithNameExistsFormat));
        }

        public List<CideEntityCategory> GetCategories(CideEngine engine)
        {
            return LoadCategories(engine).Values.Select(c => c.Category).ToList();
        }

        private Dictionary<string, EntityCategory> LoadCategories(CideEngine engine)
        {
            Debug.Assert(engine != null);

            if (_categories != null)
                return _categories;

            int count = engine.GetCategoryCount();
            _categories = new Dictionary<string, EntityCategory>(count);
            for (int i = 0; i < count; i++)
            {
                var category = new EntityCategory(engine, i);
                _categories.Add(category.Category.Name, category);
            }

            return _categories;
        }
    }
}
