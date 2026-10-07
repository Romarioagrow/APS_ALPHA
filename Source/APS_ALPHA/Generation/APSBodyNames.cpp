#include "APSBodyNames.h"

#include "HAL/PlatformTime.h"
#include "Math/RandomStream.h"
#include "Misc/Crc.h"

namespace APSBodyNamesPrivate
{
	struct FFamily
	{
		TArray<FString> Onsets;
		TArray<FString> Vowels;
		TArray<FString> Diphthongs;
		TArray<FString> Codas;
		/** Inserted between two vowels that do not form a diphthong. */
		TArray<FString> Connectors;
		TArray<FString> Endings;
		/** Japanese-like family: whole syllables instead of onset + vowel. */
		TArray<FString> Syllables;
	};

	const TArray<FFamily>& Families()
	{
		static const TArray<FFamily> Value = {
			// Aurelian (Latin): Ceria, Flosum, Riolis, Trenum
			{{TEXT("c"), TEXT("v"), TEXT("l"), TEXT("m"), TEXT("n"), TEXT("s"), TEXT("t"), TEXT("r"), TEXT("d"), TEXT("f"),
				TEXT("p"), TEXT("qu"), TEXT("pr"), TEXT("tr"), TEXT("st"), TEXT("cl"), TEXT("fl"), TEXT("gr"), TEXT("br"),
				TEXT(""), TEXT("c"), TEXT("l"), TEXT("v"), TEXT("s"), TEXT("t")},
				{TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o"), TEXT("u"), TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o")},
				{TEXT("ae"), TEXT("ia"), TEXT("io")}, {TEXT("n"), TEXT("r"), TEXT("l"), TEXT("s")},
				{TEXT("r"), TEXT("l"), TEXT("n"), TEXT("s"), TEXT("v")},
				{TEXT("a"), TEXT("ia"), TEXT("us"), TEXT("um"), TEXT("is"), TEXT("on"), TEXT("ara"), TEXT("era"), TEXT("ora"),
					TEXT("ina"), TEXT("ius"), TEXT("ea"), TEXT("ens"), TEXT("or"), TEXT("ix"), TEXT("ana"), TEXT("ica")}, {}},
			// Hellenic (Greek): Kalora, Tosion, Saphorene, Khylia
			{{TEXT("th"), TEXT("ph"), TEXT("k"), TEXT("l"), TEXT("m"), TEXT("n"), TEXT("r"), TEXT("s"), TEXT("t"), TEXT("d"),
				TEXT("z"), TEXT("kr"), TEXT("pl"), TEXT("st"), TEXT("h"), TEXT(""), TEXT("k"), TEXT("l"), TEXT("th"), TEXT("kh")},
				{TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o"), TEXT("a"), TEXT("e"), TEXT("o"), TEXT("y")},
				{TEXT("ei"), TEXT("ai"), TEXT("eo")}, {TEXT("n"), TEXT("s"), TEXT("r"), TEXT("l")},
				{TEXT("l"), TEXT("r"), TEXT("n"), TEXT("s"), TEXT("th")},
				{TEXT("os"), TEXT("is"), TEXT("ion"), TEXT("ea"), TEXT("ope"), TEXT("ene"), TEXT("ys"), TEXT("as"), TEXT("eia"),
					TEXT("on"), TEXT("ia"), TEXT("eus"), TEXT("ora"), TEXT("ax")}, {}},
			// Nordic: Skagard, Drustad, Valvik, Riborg
			{{TEXT("b"), TEXT("br"), TEXT("d"), TEXT("dr"), TEXT("f"), TEXT("fr"), TEXT("g"), TEXT("gr"), TEXT("h"), TEXT("k"),
				TEXT("l"), TEXT("m"), TEXT("n"), TEXT("r"), TEXT("s"), TEXT("sk"), TEXT("st"), TEXT("sv"), TEXT("t"), TEXT("th"),
				TEXT("v"), TEXT("h"), TEXT("s"), TEXT("v"), TEXT("b"), TEXT("bj")},
				{TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o"), TEXT("u"), TEXT("a"), TEXT("e"), TEXT("y")},
				{TEXT("ei"), TEXT("au")}, {TEXT("n"), TEXT("r"), TEXT("l"), TEXT("rn"), TEXT("ld"), TEXT("nd"), TEXT("rk"), TEXT("g")},
				{TEXT("r"), TEXT("n"), TEXT("l"), TEXT("v"), TEXT("g")},
				{TEXT("heim"), TEXT("gard"), TEXT("vik"), TEXT("dal"), TEXT("und"), TEXT("ar"), TEXT("ir"), TEXT("borg"),
					TEXT("fell"), TEXT("mund"), TEXT("stad"), TEXT("holm"), TEXT("a"), TEXT("in")}, {}},
			// Slavic: Meneva, Pimir, Broleva, Zemir
			{{TEXT("b"), TEXT("v"), TEXT("g"), TEXT("d"), TEXT("zh"), TEXT("z"), TEXT("k"), TEXT("l"), TEXT("m"), TEXT("n"),
				TEXT("p"), TEXT("r"), TEXT("s"), TEXT("t"), TEXT("ch"), TEXT("sh"), TEXT("sl"), TEXT("st"), TEXT("vl"), TEXT("dr"),
				TEXT("br"), TEXT("sv"), TEXT("kr"), TEXT("pr"), TEXT("v"), TEXT("m"), TEXT("r"), TEXT("l")},
				{TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o"), TEXT("u"), TEXT("a"), TEXT("o"), TEXT("e")},
				{TEXT("ya"), TEXT("yu")}, {TEXT("n"), TEXT("r"), TEXT("l"), TEXT("v")},
				{TEXT("v"), TEXT("r"), TEXT("l"), TEXT("n"), TEXT("sl")},
				{TEXT("ava"), TEXT("ina"), TEXT("ograd"), TEXT("slav"), TEXT("ovo"), TEXT("ka"), TEXT("mir"), TEXT("ana"),
					TEXT("ets"), TEXT("iya"), TEXT("eva"), TEXT("ich"), TEXT("ena"), TEXT("ozh")}, {}},
			// Japanese-like: Kurishi, Takida, Hidara, Mokeri
			{{}, {}, {}, {}, {},
				{TEXT(""), TEXT(""), TEXT(""), TEXT("n"), TEXT("ra"), TEXT("ri"), TEXT("ko"), TEXT("to"), TEXT("ma"), TEXT("mi"),
					TEXT("ya"), TEXT("da"), TEXT("shi"), TEXT("ne")},
				{TEXT("ka"), TEXT("ki"), TEXT("ku"), TEXT("ke"), TEXT("ko"), TEXT("sa"), TEXT("shi"), TEXT("su"), TEXT("se"),
					TEXT("so"), TEXT("ta"), TEXT("chi"), TEXT("tsu"), TEXT("te"), TEXT("to"), TEXT("na"), TEXT("ni"), TEXT("no"),
					TEXT("ha"), TEXT("hi"), TEXT("fu"), TEXT("ho"), TEXT("ma"), TEXT("mi"), TEXT("mo"), TEXT("ya"), TEXT("yu"),
					TEXT("yo"), TEXT("ra"), TEXT("ri"), TEXT("ru"), TEXT("re"), TEXT("ro"), TEXT("wa"), TEXT("ga"), TEXT("go"),
					TEXT("da"), TEXT("do"), TEXT("ba"), TEXT("ze"), TEXT("ji")}},
			// Stellar: Zeryx, Vekaron, Sacaron, Nakeix
			{{TEXT("z"), TEXT("v"), TEXT("k"), TEXT("n"), TEXT("r"), TEXT("l"), TEXT("t"), TEXT("s"), TEXT("c"), TEXT("v"),
				TEXT("z"), TEXT("k"), TEXT("l"), TEXT("s"), TEXT("t")},
				{TEXT("a"), TEXT("e"), TEXT("i"), TEXT("o"), TEXT("u"), TEXT("a"), TEXT("e"), TEXT("y")},
				{TEXT("ae"), TEXT("eo")}, {TEXT("x"), TEXT("n"), TEXT("r"), TEXT("l"), TEXT("s"), TEXT("th")},
				{TEXT("r"), TEXT("l"), TEXT("n"), TEXT("v"), TEXT("z")},
				{TEXT("ax"), TEXT("ion"), TEXT("ex"), TEXT("ara"), TEXT("ix"), TEXT("on"), TEXT("ys"), TEXT("eon"), TEXT("ira"),
					TEXT("ux"), TEXT("ar"), TEXT("is"), TEXT("yx"), TEXT("or")}, {}},
			// Qadiri (Arabic-like): Shanir, Didar, Tumul, Khundian
			{{TEXT("q"), TEXT("kh"), TEXT("z"), TEXT("s"), TEXT("sh"), TEXT("r"), TEXT("m"), TEXT("n"), TEXT("b"), TEXT("d"),
				TEXT("f"), TEXT("h"), TEXT("j"), TEXT("t"), TEXT("l"), TEXT("k"), TEXT("s"), TEXT("r"), TEXT("")},
				{TEXT("a"), TEXT("i"), TEXT("u"), TEXT("a"), TEXT("a"), TEXT("i")},
				{TEXT("ei")}, {TEXT("r"), TEXT("n"), TEXT("m"), TEXT("l"), TEXT("d")},
				{TEXT("r"), TEXT("m"), TEXT("n"), TEXT("d"), TEXT("h")},
				{TEXT("ar"), TEXT("an"), TEXT("ir"), TEXT("im"), TEXT("ah"), TEXT("ul"), TEXT("iya"), TEXT("ad"), TEXT("un"),
					TEXT("ara"), TEXT("is"), TEXT("een"), TEXT("esh")}, {}},
		};
		return Value;
	}

	// Grand families for galaxies: Latin, Greek, stellar.
	constexpr int32 GalaxyFamilies[] = {0, 1, 5};

	bool IsVowel(const TCHAR Character)
	{
		switch (FChar::ToLower(Character))
		{
		case TEXT('a'): case TEXT('e'): case TEXT('i'): case TEXT('o'): case TEXT('u'): case TEXT('y'): return true;
		default: return false;
		}
	}

	bool IsDiphthong(const FString& Pair)
	{
		static const TCHAR* const Allowed[] = {TEXT("ae"), TEXT("ai"), TEXT("ei"), TEXT("ia"), TEXT("io"), TEXT("eo"),
			TEXT("au"), TEXT("ea"), TEXT("ya"), TEXT("yu"), TEXT("ie"), TEXT("oa")};
		for (const TCHAR* Candidate : Allowed)
		{
			if (Pair == Candidate) return true;
		}
		return false;
	}

	bool IsAllowedCluster(const FString& Cluster)
	{
		static const TCHAR* const Allowed[] = {TEXT("str"), TEXT("ndr"), TEXT("nst"), TEXT("rst"), TEXT("ngr"), TEXT("rnd"),
			TEXT("ldr"), TEXT("rkh"), TEXT("nth"), TEXT("rth"), TEXT("lth"), TEXT("sth"), TEXT("rsk"), TEXT("nsk"), TEXT("rbr"),
			TEXT("rgr"), TEXT("lgr"), TEXT("nkr"), TEXT("rkr"), TEXT("ltr"), TEXT("ntr"), TEXT("lfr"), TEXT("nfr"), TEXT("rfr"),
			TEXT("lst"), TEXT("nzh"), TEXT("rzh"), TEXT("nch"), TEXT("rch"), TEXT("nsh"), TEXT("rsh"), TEXT("lsh"), TEXT("lch"),
			TEXT("rdr"), TEXT("ndh"), TEXT("rsl"), TEXT("nsl"), TEXT("lsl"), TEXT("rvl"), TEXT("nvl"), TEXT("rpl"), TEXT("lpl"),
			TEXT("nph"), TEXT("rph"), TEXT("lph"), TEXT("rkl"), TEXT("nkl"), TEXT("rcl"), TEXT("ncl"), TEXT("lcl"), TEXT("nqu"),
			TEXT("rqu"), TEXT("lqu"), TEXT("rsv"), TEXT("nsv"), TEXT("lsv"), TEXT("rbj")};
		for (const TCHAR* Candidate : Allowed)
		{
			if (Cluster == Candidate) return true;
		}
		return false;
	}

	/** Unfortunate words in English and Russian transliteration never reach a planet. */
	bool IsBlocked(const FString& Lower)
	{
		static const TCHAR* const Blocked[] = {TEXT("fuck"), TEXT("shit"), TEXT("cunt"), TEXT("dick"), TEXT("cock"),
			TEXT("nazi"), TEXT("anal"), TEXT("anus"), TEXT("rape"), TEXT("porn"), TEXT("piss"), TEXT("slut"), TEXT("whor"),
			TEXT("fag"), TEXT("nig"), TEXT("kkk"), TEXT("sex"), TEXT("cum"), TEXT("tit"), TEXT("ass"), TEXT("poo"),
			TEXT("fart"), TEXT("butt"), TEXT("boob"), TEXT("khuy"), TEXT("huy"), TEXT("hui"), TEXT("pizd"), TEXT("pisd"),
			TEXT("bly"), TEXT("ebl"), TEXT("yob"), TEXT("ebu"), TEXT("eba"), TEXT("suka"), TEXT("srak"), TEXT("govn"),
			TEXT("zhop"), TEXT("jop"), TEXT("mudak"), TEXT("pidor"), TEXT("pidar"), TEXT("mand"), TEXT("derm"), TEXT("hren"),
			TEXT("gavn"), TEXT("cyka"), TEXT("sosi"), TEXT("huj")};
		for (const TCHAR* Candidate : Blocked)
		{
			if (Lower.Contains(Candidate)) return true;
		}
		return false;
	}

	bool IsPleasant(const FString& Word)
	{
		const FString Lower = Word.ToLower();
		if (Lower.Len() < 4 || Lower.Len() > 10 || IsBlocked(Lower)) return false;
		int32 VowelRun = 0;
		int32 ConsonantRun = 0;
		for (int32 Index = 0; Index < Lower.Len(); ++Index)
		{
			const bool bVowel = IsVowel(Lower[Index]);
			VowelRun = bVowel ? VowelRun + 1 : 0;
			ConsonantRun = bVowel ? 0 : ConsonantRun + 1;
			if (VowelRun >= 3 || ConsonantRun >= 4) return false;
			if (VowelRun == 2 && !IsDiphthong(Lower.Mid(Index - 1, 2))) return false;
			if (ConsonantRun == 3 && !IsAllowedCluster(Lower.Mid(Index - 2, 3))) return false;
			if (Index >= 2 && Lower[Index] == Lower[Index - 1] && Lower[Index] == Lower[Index - 2]) return false;
		}
		// A repeated chunk ("lala", "zezex") reads like a stutter.
		for (int32 Length = 2; Length * 2 <= Lower.Len(); ++Length)
		{
			for (int32 Start = 0; Start + Length * 2 <= Lower.Len(); ++Start)
			{
				if (Lower.Mid(Start, Length) == Lower.Mid(Start + Length, Length)) return false;
			}
		}
		static const TCHAR* const BadEndings[] = {TEXT("q"), TEXT("j"), TEXT("kh"), TEXT("sh"), TEXT("zh"), TEXT("ch"),
			TEXT("w"), TEXT("bj")};
		for (const TCHAR* Ending : BadEndings)
		{
			if (Lower.EndsWith(Ending)) return false;
		}
		return true;
	}

	const FString& Pick(FRandomStream& Random, const TArray<FString>& Values)
	{
		return Values[Random.RandRange(0, Values.Num() - 1)];
	}

	/** Joins two parts; two vowels that are no diphthong get one of the family's connector consonants. */
	FString Join(const FString& Left, const FString& Right, const FFamily& Family, FRandomStream& Random)
	{
		if (Left.IsEmpty() || Right.IsEmpty() || Family.Connectors.IsEmpty()) return Left + Right;
		const TCHAR Last = Left[Left.Len() - 1];
		const bool bHiatus = IsVowel(Last) && IsVowel(Right[0]);
		const bool bLongRun = bHiatus && Left.Len() > 1 && IsVowel(Left[Left.Len() - 2]);
		if (bHiatus && (bLongRun || !IsDiphthong(FString::Chr(FChar::ToLower(Last)) + FString::Chr(Right[0]))))
		{
			return Left + Pick(Random, Family.Connectors) + Right;
		}
		return Left + Right;
	}

	FString Capitalize(const FString& Word)
	{
		FString Result = Word.ToLower();
		if (!Result.IsEmpty()) Result[0] = FChar::ToUpper(Result[0]);
		return Result;
	}

	/** One word of the family. Stem (lower case) starts it when not empty: a moon that sounds like its planet. */
	FString Word(const FFamily& Family, FRandomStream& Random, const FString& Stem)
	{
		FString Candidate;
		for (int32 Attempt = 0; Attempt < 32; ++Attempt)
		{
			if (!Family.Syllables.IsEmpty())
			{
				const int32 Count = (Random.RandRange(0, 2) == 2 ? 3 : 2) - (Stem.IsEmpty() ? 0 : 1);
				Candidate = Stem;
				for (int32 Index = 0; Index < Count; ++Index) Candidate += Pick(Random, Family.Syllables);
				Candidate += Pick(Random, Family.Endings);
			}
			else
			{
				const int32 Count = !Stem.IsEmpty() || Random.FRand() < 0.55f ? 1 : 2;
				Candidate = Stem;
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const bool bFirst = Index == 0 && Stem.IsEmpty();
					FString Syllable = Pick(Random, Family.Onsets);
					Syllable += Random.FRand() < 0.10f && !Family.Diphthongs.IsEmpty()
						? Pick(Random, Family.Diphthongs) : Pick(Random, Family.Vowels);
					if (Random.FRand() < (bFirst ? 0.18f : 0.25f)) Syllable += Pick(Random, Family.Codas);
					Candidate = Join(Candidate, Syllable, Family, Random);
				}
				Candidate = Join(Candidate, Pick(Random, Family.Endings), Family, Random);
			}
			if (IsPleasant(Candidate)) break;
		}
		return Capitalize(Candidate);
	}

	/** The leading consonants and first vowel: "Kalora" -> "ka". */
	FString StemOf(const FString& Name)
	{
		const FString Lower = Name.ToLower();
		for (int32 Index = 0; Index < Lower.Len(); ++Index)
		{
			if (IsVowel(Lower[Index])) return Lower.Left(Index + 1);
		}
		return Lower.Left(2);
	}

	FRandomStream Stream(const int32 WorldSeed, const FString& Address, const TCHAR* Channel)
	{
		const FString Key = FString::Printf(TEXT("%d/%s/%s"), WorldSeed, *Address, Channel);
		return FRandomStream(static_cast<int32>(FCrc::StrCrc32(*Key) & 0x7fffffff));
	}

	/** The address up to (not including) the last occurrence of Marker: "SYS0/S1/P2" with "/P" -> "SYS0/S1". */
	FString Parent(const FString& Address, const TCHAR* Marker)
	{
		const int32 Index = Address.Find(Marker, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return Index == INDEX_NONE ? FString() : Address.Left(Index);
	}

	int32 StarFamily(const int32 WorldSeed, const FString& StarAddress)
	{
		// Every star of a system shares one family, so a binary reads as one place.
		const FString System = Parent(StarAddress, TEXT("/S"));
		FRandomStream Random = Stream(WorldSeed, System.IsEmpty() ? StarAddress : System, TEXT("family.v1"));
		return Random.RandRange(0, Families().Num() - 1);
	}

	int32 PlanetFamily(const int32 WorldSeed, const FString& PlanetAddress)
	{
		// Most planets follow their star; some were named by someone else.
		FRandomStream Random = Stream(WorldSeed, PlanetAddress, TEXT("family.v1"));
		const int32 Own = Random.RandRange(0, Families().Num() - 1);
		return Random.FRand() < 0.6f ? StarFamily(WorldSeed, Parent(PlanetAddress, TEXT("/P"))) : Own;
	}

	FString PlanetWord(const int32 WorldSeed, const FString& PlanetAddress)
	{
		FRandomStream Random = Stream(WorldSeed, PlanetAddress, TEXT("name.v1"));
		return Word(Families()[PlanetFamily(WorldSeed, PlanetAddress)], Random, FString());
	}
}

FString APSBodyNames::Generate(const int32 WorldSeed, const FString& Address, const EKind Kind)
{
	using namespace APSBodyNamesPrivate;
	FRandomStream Random = Stream(WorldSeed, Address, TEXT("name.v1"));
	switch (Kind)
	{
	case EKind::Galaxy:
	{
		const int32 Family = GalaxyFamilies[Random.RandRange(0, UE_ARRAY_COUNT(GalaxyFamilies) - 1)];
		return Word(Families()[Family], Random, FString());
	}
	case EKind::Cluster:
		return Word(Families()[Random.RandRange(0, Families().Num() - 1)], Random, FString());
	case EKind::Star:
		return Word(Families()[StarFamily(WorldSeed, Address)], Random, FString());
	case EKind::Planet:
		return PlanetWord(WorldSeed, Address);
	case EKind::Moon:
	default:
	{
		// Same family as the planet; two moons in five also start like it (Kalora -> Kaleus).
		const FString PlanetAddress = Parent(Address, TEXT("/M"));
		const FFamily& Family = Families()[PlanetFamily(WorldSeed, PlanetAddress)];
		const FString Stem = Random.FRand() < 0.4f ? StemOf(PlanetWord(WorldSeed, PlanetAddress)) : FString();
		return Word(Family, Random, Stem);
	}
	}
}

FString APSBodyNames::Legacy(const int32 WorldSeed, const FString& Address, const FString& LegacyKind)
{
	// Byte-for-byte the pre-02.10 APSGeneratedBodyIdentity::Name, so saved worlds keep their names and fleet keys.
	FRandomStream Random = APSBodyNamesPrivate::Stream(WorldSeed, Address, TEXT("name"));
	const FString Vowels = TEXT("aeiou");
	const FString Consonants = TEXT("bcdfghjklmnpqrstvwxyz");
	FString Word;
	const int32 Length = Random.RandRange(3, 8);
	for (int32 Index = 0; Index < Length; ++Index)
	{
		const FString& Alphabet = Index % 2 == 0 ? Consonants : Vowels;
		Word += Alphabet[Random.RandRange(0, Alphabet.Len() - 1)];
	}
	Word[0] = FChar::ToUpper(Word[0]);
	return Word + TEXT(" ") + LegacyKind;
}

FString APSBodyNames::Random(const EKind Kind)
{
	using namespace APSBodyNamesPrivate;
	static uint32 Counter = 0;
	FRandomStream Random(static_cast<int32>((FPlatformTime::Cycles() ^ (++Counter * 2654435761u)) & 0x7fffffff));
	const int32 Family = Kind == EKind::Galaxy
		? GalaxyFamilies[Random.RandRange(0, UE_ARRAY_COUNT(GalaxyFamilies) - 1)]
		: Random.RandRange(0, Families().Num() - 1);
	return Word(Families()[Family], Random, FString());
}

APSBodyNames::EKind APSBodyNames::KindFromLegacy(const FString& LegacyKind)
{
	if (LegacyKind.Equals(TEXT("Planet"), ESearchCase::IgnoreCase)) return EKind::Planet;
	if (LegacyKind.Equals(TEXT("Moon"), ESearchCase::IgnoreCase)) return EKind::Moon;
	return EKind::Star;
}

FName APSBodyNames::ForStyle(const int32 Style, const int32 WorldSeed, const FString& Address, const FString& LegacyKind)
{
	return FName(*(Style == LegacyStyle ? Legacy(WorldSeed, Address, LegacyKind)
		: Generate(WorldSeed, Address, KindFromLegacy(LegacyKind))));
}
