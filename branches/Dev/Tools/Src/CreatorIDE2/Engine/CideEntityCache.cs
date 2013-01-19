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

        public event EventHandler<CideEntityPropertyChangedEventArgs> EntityPropertyChanged;
        public event EventHandler<CideEntityPropertyChangedEventArgs> CategoryPropertyChanged;

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
            cat.Entity.PropertyChanged += OnCategoryPropertyChanged;

            return cat.Entity;
        }

        private void OnCategoryPropertyChanged(object sender, PropertyChangedEventArgs eArgs)
        {
            var h = CategoryPropertyChanged;
            if (h != null)
                h(this, (CideEntityPropertyChangedEventArgs)eArgs);
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
                _categories.Add(category.Category.UID, category);
            }

            return _categories;
        }

        public List<CideEntity> GetEntities(CideEngine engine)
        {
            return LoadEntities(engine).Values.ToList();
        }

        private Dictionary<string, CideEntity> LoadEntities(CideEngine engine)
        {
            Debug.Assert(engine != null);

            if (_entities != null)
                return _entities;

            int count = engine.GetEntityCount();
            _entities = new Dictionary<string, CideEntity>(count);
            var categories = LoadCategories(engine);
            for(int i=0; i<count; i++)
            {
                var categoryUID = engine.GetEntityCategory(i);

                EntityCategory category;
                if (categoryUID == null || !categories.TryGetValue(categoryUID, out category))
                    continue; // Ignore an entity if it's category was not loaded to the cache.

                var uid = engine.GetEntityUID(i);
                Debug.Assert(!string.IsNullOrEmpty(uid));

                var entity = new CideEntity(engine, category.Category, uid, true);
                entity.UIDChanging += OnEntityUIDChanging;
                entity.PropertyChanged += OnEntityPropertyChanged;

                _entities.Add(uid, entity);
            }

            return _entities;
        }

        private void OnEntityPropertyChanged(object sender, PropertyChangedEventArgs eArgs)
        {
            var e = (CideEntityPropertyChangedEventArgs) eArgs;
            if (e.PropertyName == CideEntity.UIDPropertyName)
            {
                Debug.Assert(e.OldValue is string && e.NewValue is string);
                CideEntity entity;
                Debug.Assert(_entities.TryGetValue((string) e.OldValue, out entity) && ReferenceEquals(entity, e.Entity));            

                _entities.Remove((string) e.OldValue);

                Debug.Assert(((string) e.NewValue) == e.Entity.UID);
                _entities.Add(e.Entity.UID, e.Entity);
            }

            var h = EntityPropertyChanged;
            if (h != null)
                h(this, e);
        }

        private void OnEntityUIDChanging(object sender, PropertyChangingCancelEventArgs<string> e)
        {
            OnEntityUIDChanging(sender, e, _entities, SR.EmptyEntityNameDisallowed, SR.EntityWithNameExistsFormat);
        }

        private static void OnEntityUIDChanging<T>(object sender, PropertyChangingCancelEventArgs<string> e, Dictionary<string, T> cache, string srEmptyName, string srNameExistsFormat)
        {
            var category = (CideEntity) sender;
            Debug.Assert(cache != null && cache.ContainsKey(category.UID));

            var newVal = e.NewValue;
            if (!string.IsNullOrEmpty(newVal))
                newVal = newVal.Trim();

            if (string.IsNullOrEmpty(newVal))
                throw new ArgumentException(SR.GetString(srEmptyName));

            if (newVal == e.OldValue)
            {
                e.Cancel = true;
                return;
            }

            if (cache.ContainsKey(newVal))
                throw new DuplicateNameException(SR.GetFormatString(srNameExistsFormat, newVal));
        }
    }
}
