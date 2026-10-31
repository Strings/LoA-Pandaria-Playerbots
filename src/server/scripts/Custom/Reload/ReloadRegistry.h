#ifndef ULTIMATEMOP_RELOAD_REGISTRY_H
#define ULTIMATEMOP_RELOAD_REGISTRY_H

#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace CustomReload
{
    struct Result
    {
        bool Success;
        std::size_t Rows;
        std::string Message;
    };

    class Registry
    {
    public:
        using Loader = std::function<Result()>;

        bool Register(std::string name, std::string description, Loader loader)
        {
            return RegisterGroup({ std::move(name) }, std::move(description), std::move(loader));
        }

        // Related tables publish one cache generation and must share a reload guard.
        bool RegisterGroup(std::vector<std::string> names, std::string description, Loader loader)
        {
            if (names.empty() || !loader)
                return false;
            std::set<std::string> unique;
            for (auto const& name : names)
                if (!ValidName(name) || !unique.insert(name).second)
                    return false;
            std::lock_guard<std::mutex> lock(_mutex);
            for (auto const& name : names)
                if (_tables.count(name))
                    return false;
            auto entry = std::make_shared<Entry>(std::move(description), std::move(loader));
            for (auto const& name : names)
                _tables.emplace(name, entry);
            return true;
        }

        Result Reload(std::string const& name)
        {
            std::shared_ptr<Entry> entry;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto found = _tables.find(name);
                if (found == _tables.end())
                    return { false, 0, "Unknown custom table; use reload custom list" };
                entry = found->second;
            }
            // No catalog lock during I/O; reject recursive or overlapping reloads.
            std::unique_lock<std::recursive_mutex> lock(entry->Mutex, std::try_to_lock);
            if (!lock.owns_lock() || entry->Reloading)
                return { false, 0, "This table is already reloading" };
            entry->Reloading = true;
            struct ResetReloading
            {
                bool& Value;
                ~ResetReloading() { Value = false; }
            } reset{ entry->Reloading };
            try
            {
                return entry->Load();
            }
            catch (std::exception const& error)
            {
                return { false, 0, error.what() };
            }
            catch (...)
            {
                return { false, 0, "Loader threw a non-standard exception" };
            }
        }

        std::vector<std::pair<std::string, std::string>> List() const
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::vector<std::pair<std::string, std::string>> result;
            for (auto const& table : _tables)
                result.emplace_back(table.first, table.second->Description);
            return result;
        }

        static bool ValidName(std::string const& name)
        {
            if (name.empty() || name.size() > 64 || name[0] < 'a' || name[0] > 'z')
                return false;
            for (char character : name)
                if (!((character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_'))
                    return false;
            return name != "custom" && name != "list";
        }

    private:
        struct Entry
        {
            Entry(std::string description, Loader loader) : Description(std::move(description)), Load(std::move(loader)) { }
            std::string const Description;
            Loader const Load;
            std::recursive_mutex Mutex;
            bool Reloading = false;
        };
        mutable std::mutex _mutex;
        std::map<std::string, std::shared_ptr<Entry>> _tables;
    };

    inline Registry& Tables()
    {
        static Registry registry;
        return registry;
    }
}

#endif
