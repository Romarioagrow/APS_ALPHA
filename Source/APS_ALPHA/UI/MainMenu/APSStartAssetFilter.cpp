#include "APSStartAssetFilter.h"

bool UAPSStartAssetFilter::Hides(const FSoftObjectPath& ClassPath) const
{
	const auto Lists = [&ClassPath](const TArray<TSoftClassPtr<AActor>>& Classes)
	{
		return Classes.ContainsByPredicate([&ClassPath](const TSoftClassPtr<AActor>& Class)
		{
			return Class.ToSoftObjectPath() == ClassPath;
		});
	};
	return Lists(ExcludedClasses) || Lists(ClassesWithoutVisuals);
}
