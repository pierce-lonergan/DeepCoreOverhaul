// Objective.cpp : 
//

#include "../../engine/audio/3DSound.h"
#include "../../engine/core/Files.h"
#include "../../engine/core/Utils.h"
#include "../../engine/input/Input.h"

#include "../audio/SFX.h"
#include "../front/FrontEnd.h"
#include "../front/Reward.h"
#include "../interface/Advisor.h"
#include "../interface/TextMessages.h"
#include "../interface/ToolTip.h"
#include "../interface/hud/Bubbles.h"
#include "../world/Construction.h"
#include "../world/Teleporter.h"
#include "../Game.h"
#include "NERPsFile.h"
#include "NERPsFunctions.h"

#include "Objective.h"


/**********************************************************************************
 ******** Globals
 **********************************************************************************/

#pragma region Globals

// <LegoRR.exe @00500bc0>
LegoRR::Objective_Globs & LegoRR::objectiveGlobs = *(LegoRR::Objective_Globs*)0x00500bc0;

#pragma endregion

/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @004577a0>
void __cdecl LegoRR::Objective_LoadObjectiveText(const Gods98::Config* config, const char* gameName, const char* levelName, Lego_Level* level, const char* filename)
{
	// HARDCODED RESOLUTION POSITIONS
	const Area2F objectiveArea = {
		(level->objective.panelImagePosition.x +  60.0f),
		(level->objective.panelImagePosition.y +  60.0f),
		390.0f,
		180.0f,
	};
	const Area2F beginArea = {
		(level->objective.panelImagePosition.x +  60.0f),
		(level->objective.panelImagePosition.y + 220.0f),
		320.0f,
		 20.0f,
	};
	const Area2F titleArea = {
		(level->objective.panelImagePosition.x +  40.0f),
		(level->objective.panelImagePosition.y +  34.0f),
		422.0f,
		 21.0f,
	};

	/// NOTE: This field is used ONLY for opening the file later down in this function. It's not necessary.
	/// CHANGE: Using "%s" is pointless when we have strcpy.
	std::strcpy(objectiveGlobs.filename, filename);
	//std::strncpy(objectiveGlobs.filename, filename, (sizeof(objectiveGlobs.filename) - 1));
	//objectiveGlobs.filename[sizeof(objectiveGlobs.filename) - 1] = '\0';
	//std::sprintf(objectiveGlobs.filename, "%s", filename);


	// Remove objective, page, and begin text windows.
	/// FIX APPLY: Free all text windows in this array. Not just the first index.
	for (uint32 i = 0; i < OBJECTIVE_MESSAGECOUNT; i++) {
		if (objectiveGlobs.textWindows[i] != nullptr) {
			Gods98::TextWindow_Remove(objectiveGlobs.textWindows[i]);
			objectiveGlobs.textWindows[i] = nullptr;
		}
	}
	if (objectiveGlobs.briefingTitleTextWindow != nullptr) {
		Gods98::TextWindow_Remove(objectiveGlobs.briefingTitleTextWindow);
		objectiveGlobs.briefingTitleTextWindow = nullptr;
	}
	if (objectiveGlobs.completedTitleTextWindow != nullptr) {
		Gods98::TextWindow_Remove(objectiveGlobs.completedTitleTextWindow);
		objectiveGlobs.completedTitleTextWindow = nullptr;
	}
	if (objectiveGlobs.failedTitleTextWindow != nullptr) {
		Gods98::TextWindow_Remove(objectiveGlobs.failedTitleTextWindow);
		objectiveGlobs.failedTitleTextWindow = nullptr;
	}
	if (objectiveGlobs.beginTextWindow != nullptr) {
		Gods98::TextWindow_Remove(objectiveGlobs.beginTextWindow);
		objectiveGlobs.beginTextWindow = nullptr;
	}

	// Free message strings and reset pages.
	for (uint32 i = 0; i < OBJECTIVE_MESSAGECOUNT; i++) {
		if (objectiveGlobs.messages[i] != nullptr) {
			Gods98::Mem_Free(objectiveGlobs.messages[i]);
			objectiveGlobs.messages[i] = nullptr;
		}
		objectiveGlobs.pageCounts[i] = 0;
		objectiveGlobs.currentPageStates[i] = 0;
		objectiveGlobs.currentPages[i] = 0;
	}


	// Create new objective text windows.
	for (uint32 i = 0; i < OBJECTIVE_MESSAGECOUNT; i++) {
		objectiveGlobs.textWindows[i] = Gods98::TextWindow_Create(legoGlobs.fontBriefingLo, &objectiveArea, 1024);
		Gods98::TextWindow_EnableCentering(objectiveGlobs.textWindows[i], false);
	}

	// Create and setup begin text window.
	objectiveGlobs.beginTextWindow = Gods98::TextWindow_Create(legoGlobs.fontBriefingHi, &beginArea, 1024);
	objectiveGlobs.hasBeginText = false;


	// Open the objective text file and search for the [BEGIN] and [ShortLevelName] sections.
	/// NOTE: This field is used ONLY for opening the file. It's not necessary when we still have the filename argument.
	objectiveGlobs.file = Gods98::File_Open(objectiveGlobs.filename, "r");
	//objectiveGlobs.file = Gods98::File_Open(filename, "r");
	if (objectiveGlobs.file != nullptr) {
		Gods98::File_Seek(objectiveGlobs.file, 0, Gods98::SeekOrigin::Set);

		/// FIXME: Really hacky solution to strip "Levels::" prefix from levelName!
		const char* shortLevelName = nullptr;
		bool hasShortLevelName = false;
		uint32 colonCount = 0;
		for (const char* s = levelName; *s != '\0'; s++) {
			/*if (colonCount == 2) {
				// We've passed the "::".
				shortLevelName = s; // levelName + i
				hasShortLevelName = true;
				break;
			}
			else*/ if (*s == ':') {
				colonCount++;
				/// CHANGE: Just check colon count now.
				// Note that using this will not fail when at the end of the string, unlike using the above check.
				if (colonCount == 2) {
					// We've passed the "::".
					shortLevelName = s + 1; // levelName + i + 1
					hasShortLevelName = true;
					break;
				}
			}
		}

		/// REFACTOR: Move this outside the loop, because it's a waste of cpu.
		/// REFACTOR: We don't need levelNameBuff, just pass shortLevelName directly.
		//char levelNameBuff[1024];
		//std::strcpy(levelNameBuff, (hasShortLevelName ? shortLevelName : ""));
		char levelSectionBuff[1024];
		const uint32 levelSectionLength = std::sprintf(levelSectionBuff, "[%s]",
													   (hasShortLevelName ? shortLevelName : "")); // levelNameBuff);

		// Prefixes for objective types in level sections.
		static const auto STATUS_PREFIXES = array_of<std::string>(
			"Objective:", "Completion:", "Failure:", "CrystalFailure:");

		const char beginSectionName[] = "[BEGIN]";
		const uint32 beginSectionLength = std::strlen(beginSectionName);

		// Note that the objective text file is expected to use LF returns only, not CRLF.

		// [ShortLevelName]
		// Objective:      Text goes here.\nThis is a newline escape.\aThis is a new page escape.
		// Completion:     Text goes here... Note that underscores are not converted to spaces.
		// Failure:        Text goes here...
		// CrystalFailure: Text goes here...

		// [BEGIN]
		// Text goes here.

		bool inLevelSection = false;
		bool inBeginSection = false;
		char buff[1024];

		/// TODO: Consider switching to File_GetLine, because it automatically handles trimming newlines.
		char* line = Gods98::File_GetS(buff, sizeof(buff), objectiveGlobs.file);
		while (line != nullptr) {
			// Note that this check also prevents reading [BEGIN] text. Is this intentional?
			if (hasShortLevelName) {
				if (buff[0] == '[') {
					// This is the beginning of a new section definition. We're no longer in these sections.
					/// TODO: Consider rewriting parser so that [BEGIN] text can start with [ bracket.
					inLevelSection = false;
					inBeginSection = false;
				}

				if (inLevelSection) {
					/// SANITY: Ensure we have a non-zero length for when we remove newline characters.
					if (buff[0] != '\0') buff[std::strlen(buff) - 1] = '\0'; // Strip the newline.

					// Find the prefix that describes the objective type this line defines (if any).
					uint32 skip = 0; // Characters to skip till after start of prefix and whitespace.
					uint32 briefIdx = 0; // Index of text window. STATUS_PREFIXES.size() == invalid.
					for (; briefIdx < STATUS_PREFIXES.size(); briefIdx++) {
						skip = STATUS_PREFIXES[briefIdx].length();
						if (::_strnicmp(buff, STATUS_PREFIXES[briefIdx].c_str(), skip) == 0) {
							break;
						}
					}

					// Check if we matched an objective type prefix.
					if (briefIdx < STATUS_PREFIXES.size()) {
						// Strip whitespace after "prefix:".
						// Note that this also increments skip.
						for (const char* s = buff + skip; *s != '\0'; s++, skip++) {
							if (*s != ' ' && *s != '\t') {
								// End of whitespace, no more to strip.
								break;
							}
						}

						// Unescape characters (newlines and page breaks).
						/// TODO: This could be rewritten to have cleaner parsing, but mods may depend on these loose rules.
						/// CHANGE: Start loop after the skipped characters, unescaping those does nothing.
						bool escape = false;
						for (char* t = buff + skip; *t != '\0'; t++) {
							// A VERY RARE case where text is case sensitive. \A and \N are not valid escapes.
							if (escape && (*t == 'a' || *t == 'n')) {
								t[-1] = ' '; // Replace escape backslash with space... I guess...?
								if (*t == 'a') {
									*t = '\a'; // Page break.
									objectiveGlobs.pageCounts[briefIdx]++;
								}
								else if (*t == 'n') {
									*t = '\n'; // New line.
								}
								escape = false;
							}
							else {
								// Note that double backslashes will not result in a single blackslash.
								escape = (*t == '\\');
							}
						}

						// Print to text window.
						Gods98::TextWindow_PrintF(objectiveGlobs.textWindows[briefIdx], "%s", buff + skip);
						objectiveGlobs.messages[briefIdx] = Gods98::Util_StrCpy(buff + skip);
					}
				}

				if (inBeginSection) {
					// Begin section only lasts for one line, and unlike level sections, the first line is ALWAYS used.
					inBeginSection = false;

					/// SANITY: Ensure we have a non-zero length for when we remove newline characters.
					if (buff[0] != '\0') buff[std::strlen(buff) - 1] = '\0'; // Strip the newline.
					// No prefix and no special parsing for begin section.
					Gods98::TextWindow_PrintF(objectiveGlobs.beginTextWindow, "%s", buff);
					objectiveGlobs.hasBeginText = true;
				}

				// Check if a new section is defined.
				/// NOTE: If a level's short name is BEGIN, then the first line should be the begin text,
				///        and remaining lines should be prefixed objective type text.
				if (::_strnicmp(buff, levelSectionBuff, levelSectionLength) == 0) {
					inLevelSection = true;
				}
				if (::_strnicmp(buff, beginSectionName, beginSectionLength) == 0) {
					inBeginSection = true;
				}
			}
			line = Gods98::File_GetS(buff, sizeof(buff), objectiveGlobs.file);
		}

		Gods98::File_Close(objectiveGlobs.file);
		objectiveGlobs.file = nullptr;
	}


	// Load text titles for objective windows.
	// The supposed buffer size for this is 128, but there's 128 + 1024*3 characters of unused buffer space available.
	// We only need to use 1024, since that's the buffer size of our text window.
	char titleBuff[1024] = { '\0' }; // dummy init
	const char* title;

	// usage: MissionBriefingText    Text_with_underscore_spaces
	title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionBriefingText"));
	if (title != nullptr) {
		// Center X.
		/// FIX APPLY: Don't keep shifting the area struct that we're reusing for each title!!
		const uint32 strWidth = Gods98::Font_GetStringWidth(legoGlobs.fontBriefingHi, title);
		/// TODO: We could refactor this to cast strWidth to a float before diving by 2, that way two 0.5's can add up to 1.
		Area2F newTitleArea = titleArea;
		newTitleArea.x += (titleArea.width / 2.0f - static_cast<real32>(strWidth / 2));
		newTitleArea.width = static_cast<real32>(strWidth);

		objectiveGlobs.briefingTitleTextWindow = Gods98::TextWindow_Create(legoGlobs.fontBriefingHi, &newTitleArea, 1024);
		if (objectiveGlobs.briefingTitleTextWindow != nullptr) {
			// Replace underscores with spaces, because we can't have spaces in config properties.
			char* t = titleBuff;
			for (const char* s = title; *s != '\0'; s++, t++) {
				if (*s == '_') *t = ' ';
				else           *t = *s;
			}
			*t = '\0'; // Null-terminate string.
			Gods98::TextWindow_Clear(objectiveGlobs.briefingTitleTextWindow);
			Gods98::TextWindow_PrintF(objectiveGlobs.briefingTitleTextWindow, "%s", titleBuff);
		}
	}

	// usage: MissionCompletedText    Text_with_underscore_spaces
	title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionCompletedText"));
	if (title != nullptr) {
		// Center X.
		/// FIX APPLY: Don't keep shifting the area struct that we're reusing for each title!!
		const uint32 strWidth = Gods98::Font_GetStringWidth(legoGlobs.fontBriefingHi, title);
		Area2F newTitleArea = titleArea;
		newTitleArea.x += (titleArea.width / 2.0f - static_cast<real32>(strWidth / 2));
		newTitleArea.width = static_cast<real32>(strWidth);

		objectiveGlobs.completedTitleTextWindow = Gods98::TextWindow_Create(legoGlobs.fontBriefingHi, &newTitleArea, 1024);
		if (objectiveGlobs.completedTitleTextWindow != nullptr) {
			// Replace underscores with spaces, because we can't have spaces in config properties.
			char* t = titleBuff;
			for (const char* s = title; *s != '\0'; s++, t++) {
				if (*s == '_') *t = ' ';
				else           *t = *s;
			}
			*t = '\0'; // Null-terminate string.
			Gods98::TextWindow_Clear(objectiveGlobs.completedTitleTextWindow);
			Gods98::TextWindow_PrintF(objectiveGlobs.completedTitleTextWindow, "%s", titleBuff);
		}
	}

	// usage: MissionFailedText    Text_with_underscore_spaces
	title = Gods98::Config_GetTempStringValue(config, Main_ID("MissionFailedText"));
	if (title != nullptr) {
		// Center X.
		/// FIX APPLY: Don't keep shifting the area struct that we're reusing for each title!!
		const uint32 strWidth = Gods98::Font_GetStringWidth(legoGlobs.fontBriefingHi, title);
		Area2F newTitleArea = titleArea;
		newTitleArea.x += (titleArea.width / 2.0f - static_cast<real32>(strWidth / 2));
		newTitleArea.width = static_cast<real32>(strWidth);

		objectiveGlobs.failedTitleTextWindow = Gods98::TextWindow_Create(legoGlobs.fontBriefingHi, &newTitleArea, 1024);
		if (objectiveGlobs.failedTitleTextWindow != nullptr) {
			// Replace underscores with spaces, because we can't have spaces in config properties.
			char* t = titleBuff;
			for (const char* s = title; *s != '\0'; s++, t++) {
				if (*s == '_') *t = ' ';
				else           *t = *s;
			}
			*t = '\0'; // Null-terminate string.
			Gods98::TextWindow_Clear(objectiveGlobs.failedTitleTextWindow);
			Gods98::TextWindow_PrintF(objectiveGlobs.failedTitleTextWindow, "%s", titleBuff);
		}
	}

	// Add the unused height of the begin text window to the objective text window if begin window is unused.
	if (!objectiveGlobs.hasBeginText) {
		Area2F newObjectiveArea = objectiveArea;
		newObjectiveArea.height += beginArea.height;
		//objectiveArea.height += beginArea.height;
		/// FIX APPLY: Don't skip the last element in the array.
		for (uint32 i = 0; i < OBJECTIVE_MESSAGECOUNT; i++) {
			Gods98::TextWindow_ChangeSize(objectiveGlobs.textWindows[i],
										  static_cast<sint32>(newObjectiveArea.width),
										  static_cast<sint32>(newObjectiveArea.height));
		}
	}
}

// <LegoRR.exe @00458000>
void __cdecl LegoRR::Objective_LoadLevel(const Gods98::Config* config, const char* gameName, const char* levelName, Lego_Level* level, uint32 screenWidth, uint32 screenHeight)
{
	char buff[1024];
	char* stringParts[60];
	char* str;

	// Memzero and setup default values.
	std::memset(&level->objective, 0, sizeof(ObjectiveData));

	objectiveGlobs.flags = OBJECTIVE_GLOB_FLAG_NONE;
	objectiveGlobs.objectiveSwitch = true;

	// Assign flags.

	// usage: DontShowObjectiveAdvisor           bool
	// usage: DontShowObjectiveAcheiveAdvisor    bool
	// usage: DontShowObjectiveFailedAdvisor     bool
	if (!Config_GetBoolOrFalse(config, Config_ID(gameName, levelName, "DontShowObjectiveAdvisor"))) {
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR;
	}
	if (!Config_GetBoolOrFalse(config, Config_ID(gameName, levelName, "DontShowObjectiveAcheiveAdvisor"))) {
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR;
	}
	if (!Config_GetBoolOrFalse(config, Config_ID(gameName, levelName, "DontShowObjectiveFailedAdvisor"))) {
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_SHOWFAILEDADVISOR;
	}

	// Assign images, and video filenames.

	// usage: ObjectiveImage<W>x<H>    filename,x,y[,rtrans,gtrans,btrans]
	std::sprintf(buff, "ObjectiveImage%ix%i", screenWidth, screenHeight);
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, buff));
	if (str != nullptr) {
		const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ",", _countof(stringParts));

		// Cleanup the old image if one exists (unreachable because of memzero).
		if (level->objective.panelImage != nullptr) {
			Gods98::Image_Remove(level->objective.panelImage);
		}
		// Load new image.
		level->objective.panelImage = Gods98::Image_LoadBMP(stringParts[0]);

		// Setup image position.
		level->objective.panelImagePosition = Point2F {
			static_cast<real32>(std::atoi(stringParts[1])),
			static_cast<real32>(std::atoi(stringParts[2])),
		};

		// Setup image transparency.
		if (level->objective.panelImage != nullptr) {
			if (argc == 6) {
				// Optional string parts: "filename,x,y[,r,g,b]"
				/// FIX APPLY: Cast to float BEFORE dividing by 255!
				const real32 r = static_cast<real32>(std::atoi(stringParts[3])) / 255.0f;
				const real32 g = static_cast<real32>(std::atoi(stringParts[4])) / 255.0f;
				const real32 b = static_cast<real32>(std::atoi(stringParts[5])) / 255.0f;
				Gods98::Image_SetupTrans(level->objective.panelImage, r, g, b, r, g, b);
			}
			else {
				Gods98::Image_SetPenZeroTrans(level->objective.panelImage);
			}
		}
		Gods98::Mem_Free(str);
	}

	// usage: ObjectiveAcheivedImage<W>x<H>    filename,x,y[,rtrans,gtrans,btrans]
	std::sprintf(buff, "ObjectiveAcheivedImage%ix%i", screenWidth, screenHeight);
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, buff));
	if (str != nullptr) {
		const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ",", _countof(stringParts));

		// Cleanup the old image if one exists (unreachable because of memzero).
		if (level->objective.achievedImage != nullptr) {
			Gods98::Image_Remove(level->objective.achievedImage);
		}
		// Load new image.
		level->objective.achievedImage = Gods98::Image_LoadBMP(stringParts[0]);

		// Setup image position.
		level->objective.achievedImagePosition = Point2F {
			static_cast<real32>(std::atoi(stringParts[1])),
			static_cast<real32>(std::atoi(stringParts[2])),
		};

		// Setup image transparency.
		if (level->objective.achievedImage != nullptr) {
			if (argc == 6) {
				// Optional string parts: "filename,x,y[,r,g,b]"
				/// FIX APPLY: Cast to float BEFORE dividing by 255!
				const real32 r = static_cast<real32>(std::atoi(stringParts[3])) / 255.0f;
				const real32 g = static_cast<real32>(std::atoi(stringParts[4])) / 255.0f;
				const real32 b = static_cast<real32>(std::atoi(stringParts[5])) / 255.0f;
				/// FIX APPLY: Setup transparency for achievedImage, not panelImage again!!
				Gods98::Image_SetupTrans(level->objective.achievedImage, r, g, b, r, g, b);
			}
			else {
				Gods98::Image_SetPenZeroTrans(level->objective.achievedImage);
			}
		}
		Gods98::Mem_Free(str);
	}

	// usage: ObjectiveAcheivedAVI    filename[,x,y]
	/// NOTE: Mispelling in property name "Acheived" instead of "Achieved".
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, "ObjectiveAcheivedAVI"));
	if (str != nullptr) {
		const uint32 argc = Gods98::Util_TokeniseSafe(str, stringParts, ",", _countof(stringParts));

		// Store video filename.
		level->objective.achievedVideoName = Gods98::Util_StrCpy(stringParts[0]);

		// Setup video position.
		if (argc == 3) {
			// Optional string parts: "filename[,x,y]"
			level->objective.achievedVideoPosition = Point2F {
				static_cast<real32>(std::atoi(stringParts[1])),
				static_cast<real32>(std::atoi(stringParts[2])),
			};
			level->objective.noAchievedVideoPosition = false;
		}
		else {
			level->objective.noAchievedVideoPosition = true;
		}
		Gods98::Mem_Free(str);
	}

	// usage: ObjectiveFailedImage<W>x<H>    filename,x,y
	std::sprintf(buff, "ObjectiveFailedImage%ix%i", screenWidth, screenHeight);
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, buff));
	if (str != nullptr) {
		Gods98::Util_TokeniseSafe(str, stringParts, ",", _countof(stringParts));

		// Cleanup the old image if one exists (unreachable because of memzero).
		if (level->objective.failedImage != nullptr) {
			Gods98::Image_Remove(level->objective.failedImage);
		}
		// Load new image.
		level->objective.failedImage = Gods98::Image_LoadBMP(stringParts[0]);

		// Setup image position.
		level->objective.failedImagePosition = Point2F {
			static_cast<real32>(std::atoi(stringParts[1])),
			static_cast<real32>(std::atoi(stringParts[2])),
		};

		// Setup image transparency.
		if (level->objective.failedImage != nullptr) {
			// Unlike other images, this one doesn't support specifying transparency rgb.
			Gods98::Image_SetPenZeroTrans(level->objective.failedImage);
		}
		Gods98::Mem_Free(str);
	}

	// Assign level objectives.

	// Assign crystal and ore objectives.
	// usage: CrystalObjective    count
	// usage: OreObjective        count
	const uint32 crystals = Config_GetIntValue(config, Config_ID(gameName, levelName, "CrystalObjective"));
	const uint32 ore      = Config_GetIntValue(config, Config_ID(gameName, levelName, "OreObjective"));
	Objective_SetCryOreObjectives(level, crystals, ore);

	// Assign timer objective.
	// usage: TimerObjective    seconds:<HitTimeFailObjective|*anything else*>
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, "TimerObjective"));
	if (str != nullptr) {
		Gods98::Util_TokeniseSafe(str, stringParts, ":", _countof(stringParts));
		const real32 timer = static_cast<real32>(std::atoi(stringParts[0]));
		const bool32 hitTimeFail = (::_stricmp(stringParts[1], "HitTimeFailObjective") == 0);
		Objective_SetTimerObjective(level, (timer * STANDARD_FRAMERATE), hitTimeFail);
		Gods98::Mem_Free(str);
	}

	// Assign object creation objective (not just building construction, right?).
	// usage: ConstructionObjective    objName
	const char* objName = Gods98::Config_GetTempStringValue(config, Config_ID(gameName, levelName, "ConstructionObjective"));
	if (objName != nullptr) {
		LegoObject_Type objType;
		LegoObject_ID objID;
		if (Lego_GetObjectByName(objName, &objType, &objID, nullptr)) {
			Objective_SetConstructionObjective(level, objType, objID);
		}
	}

	// Assign reach-this-block position objective.
	// usage: BlockObjective    bx,by
	str = Gods98::Config_GetStringValue(config, Config_ID(gameName, levelName, "BlockObjective"));
	if (str != nullptr) {
		Gods98::Util_TokeniseSafe(str, stringParts, ",", _countof(stringParts));
		const Point2I blockPos = {
			std::atoi(stringParts[0]),
			std::atoi(stringParts[1]),
		};
		Objective_SetBlockObjective(level, &blockPos);
		Gods98::Mem_Free(str);
	}

	// Assign objective briefing window text.
	// usage: ObjectiveText    filename
	const char* filename = Gods98::Config_GetTempStringValue(config, Config_ID(gameName, levelName, "ObjectiveText"));
	if (filename != nullptr) {
		Objective_LoadObjectiveText(config, gameName, levelName, level, filename);
	}

	// Use default flags if none defined. Is this really necessary...? Why limit the config like that?
	if (objectiveGlobs.flags == OBJECTIVE_GLOB_FLAG_NONE) {
		objectiveGlobs.flags = OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR;
	}
}

// <LegoRR.exe @00458840>
void __cdecl LegoRR::Objective_SetCryOreObjectives(Lego_Level* level, uint32 crystals, uint32 ore)
{
	if (crystals > 0) {
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_CRYSTAL;
		level->objective.crystals = crystals;
	}
	if (ore > 0) {
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_ORE;
		level->objective.ore = ore;
	}
}

// <LegoRR.exe @00458880>
void __cdecl LegoRR::Objective_SetBlockObjective(Lego_Level* level, const Point2I* blockPos)
{
	objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_BLOCK;
	level->objective.blockPos = *blockPos;
}

// <LegoRR.exe @004588b0>
void __cdecl LegoRR::Objective_SetTimerObjective(Lego_Level* level, real32 timer, bool32 hitTimeFail)
{
	objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_TIMER;
	level->objective.timer = timer;
	if (hitTimeFail) {
		/// NOTE: This has to be a bug, or a lazy workaround for bugs with the original flag.
		//objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR;
		/// FIX APPLY: Switch to HITTIMEFAIL flag.
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_HITTIMEFAIL;
	}
}

// <LegoRR.exe @004588e0>
void __cdecl LegoRR::Objective_SetConstructionObjective(Lego_Level* level, LegoObject_Type objType, LegoObject_ID objID)
{
	objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_CONSTRUCTION;
	level->objective.constructionType = objType;
	level->objective.constructionID   = objID;
}

// <LegoRR.exe @00458910>
bool32 __cdecl LegoRR::Objective_IsObjectiveAchieved(void)
{
	return objectiveGlobs.achieved;
}

// <LegoRR.exe @00458920>
void __cdecl LegoRR::Objective_SetEndTeleportEnabled(bool32 on)
{
	objectiveGlobs.endTeleportEnabled = on;
}

// <LegoRR.exe @00458930>
void __cdecl LegoRR::Objective_SetStatus(LevelStatus status)
{
	if (!NERPs_AnyTutorialFlags()) {
		legoGlobs.flags2 |= GAME2_INMENU;
	}

	// Reset everything to the first page.
	for (uint32 i = 0; i < OBJECTIVE_MESSAGECOUNT; i++) {
		objectiveGlobs.currentPages[i] = 0;
	}

	// If already showing, then SetStatus does nothing.
	if (Objective_IsShowing())
		return;
	// What's the purpose of the OBJECTIVE_GLOB_FLAG_CRYSTAL flag check??
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL)
		return;

	// Do infinite loop while waiting for user MOUSE input.
	/// TODO: Include press any key?
	while (Gods98::mslb() || Gods98::msrb() || Gods98::mslbheld() || Gods98::Input_LClicked()) {
		Gods98::INPUT.lClicked = false;
		Gods98::Main_LoopUpdate(true);
	}


	Lego_Level* level = Lego_GetLevel();
	/// TYPO: "Acheived" instead of "Achieved", can't change this because of the Lego.cfg game data.
	const char* sfxTypeName = nullptr; // "", "Acheived", "Failed", "FailedCrystals"
	bool levelEnding      = true;
	bool levelSpecificSFX = true; // When false, play a generic 'Stream_Objective' SFX that isn't tied to level name.

	switch (status) {
	case LEVELSTATUS_INCOMPLETE:
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_BRIEFING;
		level->status = LEVELSTATUS_INCOMPLETE;
		sfxTypeName      = "";
		levelEnding      = false;
		levelSpecificSFX = true;
		break;

	case LEVELSTATUS_COMPLETE:
		objectiveGlobs.achieved = true;
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_COMPLETED;
		level->status = LEVELSTATUS_COMPLETE;
		sfxTypeName      = "Acheived"; // Typo
		levelEnding      = true;
		levelSpecificSFX = true;

		// Capture the current view of the level to use as our save game thumbnail.
		Gods98::Image_GetScreenshot(&rewardGlobs.current.saveCaptureImage,
									frontGlobs.saveImageBigSize.width, frontGlobs.saveImageBigSize.height);
		rewardGlobs.current.saveHasCapture = true;

		// Cleans up some text-related stuff and game flags.
		Lego_UnkObjective_CompleteSub_FUN_004262f0();
		RewardQuota_CountUnits();
		break;

	case LEVELSTATUS_FAILED:
		objectiveGlobs.achieved = false;
		objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_FAILED;
		level->status = LEVELSTATUS_FAILED;
		sfxTypeName      = "Failed";
		levelEnding      = true;
		levelSpecificSFX = true;

		// Cleans up some text-related stuff and game flags.
		Lego_UnkObjective_CompleteSub_FUN_004262f0();
		RewardQuota_CountUnits();
		break;

	case LEVELSTATUS_FAILED_CRYSTALS:
		objectiveGlobs.achieved = false;
		objectiveGlobs.flags |= (OBJECTIVE_GLOB_FLAG_FAILED|OBJECTIVE_GLOB_FLAG_CRYSTAL);
		level->status = LEVELSTATUS_FAILED;
		sfxTypeName      = "FailedCrystals";
		levelEnding      = true;
		levelSpecificSFX = false;

		// Cleans up some text-related stuff and game flags.
		Lego_UnkObjective_CompleteSub_FUN_004262f0();
		RewardQuota_CountUnits();
		break;
	}

	objectiveGlobs.flags |= OBJECTIVE_GLOB_FLAG_STATUSREADY;

	if (levelEnding) {
		const uint32 modeFlags = 0x2;
		const uint32 teleFlags = (objectiveGlobs.endTeleportEnabled ? 0x2 : 0x4);
		/// REFACTOR: Since we're no-longer hardcoding the use of teleported object types,
		///            enumerate over the flags and find what types to teleport.
		// Start at 1 to skip LegoObject_None.
		for (uint32 i = 1; i < LegoObject_Type_Count; i++) {
			const LegoObject_TypeFlags objTypeFlag = LegoObject_TypeToFlag(static_cast<LegoObject_Type>(i));
			// is this flag is one of the teleported object types?
			if (objTypeFlag & OBJECT_TYPE_FLAGS_TELEPORTED) {
				Teleporter_Start(objTypeFlag, modeFlags, teleFlags);
			}
		}

		Construction_DisableCryOreDrop(true); // Don't spawn resource costs when tele'ing up buildings/vehicles.
		LegoObject_SetLevelEnding(true);
	}

	if (sfxTypeName != nullptr && (legoGlobs.flags1 & GAME1_USESFX)) {
		char buff[256];
		if (levelSpecificSFX) {
			std::sprintf(buff, "Stream_Objective%s_%s", sfxTypeName, level->name);
		}
		else {
			std::sprintf(buff, "Stream_Objective%s", sfxTypeName);
		}

		// Free the previous soundName if one exists.
		if (objectiveGlobs.soundName) Gods98::Mem_Free(objectiveGlobs.soundName);
		objectiveGlobs.soundName = Gods98::Util_StrCpy(buff);
		objectiveGlobs.soundHandle = -1;
		objectiveGlobs.showing = false;
	}
	else {
		/// FIX APPLY: Free the previous soundName if one exists.
		if (objectiveGlobs.soundName) Gods98::Mem_Free(objectiveGlobs.soundName);
		objectiveGlobs.soundName = nullptr;
	}
}

// <LegoRR.exe @00458ba0>
void __cdecl LegoRR::Objective_StopShowing(void)
{
	// End the current showing objective.
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {

		// nerpsruntimeGlobs.objectiveSwitch = objectiveGlobs.objectiveSwitch;
		//  (before we set ours to false)
		NERPFunc__SetObjectiveSwitch(&objectiveGlobs.objectiveSwitch); // This *should* be true at this point in the program.

		objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_BRIEFING;
		objectiveGlobs.objectiveSwitch = false;

		// We're entering the level, so start playing music, if enabled.
		/// CHANGE: This now also checks Lego_IsMusicPlaying.
		Lego_ChangeMusicPlaying(true); // Start music.
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {

		objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_COMPLETED;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {

		objectiveGlobs.flags &= ~(OBJECTIVE_GLOB_FLAG_FAILED|OBJECTIVE_GLOB_FLAG_CRYSTAL); // Clear failed and potential failed reason flags.
	}

	Advisor_End();

	if (objectiveGlobs.soundHandle != -1) {
		Gods98::Sound3D_Stream_Stop(false);
		objectiveGlobs.soundHandle = -1;
	}
	Lego_SetPaused(false, false);
	SFX_SetQueueMode_AndFlush(false);

	if (!NERPs_AnyTutorialFlags()) {
		legoGlobs.flags2 &= ~GAME2_INMENU;
	}
}

// Returns true if the mission briefing or failure/complete window is showing.
// <LegoRR.exe @00458c60>
bool32 __cdecl LegoRR::Objective_IsShowing(void)
{
	return (objectiveGlobs.flags & (OBJECTIVE_GLOB_FLAG_BRIEFING|OBJECTIVE_GLOB_FLAG_COMPLETED|OBJECTIVE_GLOB_FLAG_FAILED));
}

// <LegoRR.exe @00458c80>
bool32 __cdecl LegoRR::Objective_HandleKeys(bool32 nextKeyPressed, bool32 leftButtonReleased, OUT bool32* exitGame)
{
	LevelStatus briefIdx;
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {
		briefIdx = LEVELSTATUS_INCOMPLETE;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {
		briefIdx = LEVELSTATUS_COMPLETE;
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {
		briefIdx = (!(objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL) ? LEVELSTATUS_FAILED
																		  : LEVELSTATUS_FAILED_CRYSTALS);
	}
	else {
		/// FIX APPLY: Handle LEVELSTATUS_FAILED_OTHER now to avoid reading outside bounds of the arrays.
		//briefIdx = LEVELSTATUS_FAILED_OTHER;
		return Objective_IsShowing(); // Should be false.
	}

	*exitGame = false;

	/// CHANGE: Used the button positions specified by Objective_Update (which is called at least once before Objective_HandleKeys).
	/// HARDCODED: UI button dimensions/positions for briefing.
	//const Point2I nextPos = { 470, 315 };
	//const Point2I prevPos = { 130, 315 };
	const Point2I nextPos = { (sint32)legoGlobs.menuNextPoint.x, (sint32)legoGlobs.menuNextPoint.y };
	const Point2I prevPos = { (sint32)legoGlobs.menuPrevPoint.x, (sint32)legoGlobs.menuPrevPoint.y };

	const bool overNext = (legoGlobs.NextButtonImage != nullptr && (legoGlobs.flags2 & GAME2_MENU_HASNEXT) &&
	                       (Gods98::msx() >= nextPos.x && Gods98::msx() < nextPos.x + (sint32)Gods98::Image_GetWidth(legoGlobs.NextButtonImage)) &&
	                       (Gods98::msy() >= nextPos.y && Gods98::msy() < nextPos.y + (sint32)Gods98::Image_GetHeight(legoGlobs.NextButtonImage)));
	
	const bool overPrev = (legoGlobs.RepeatButtonImage != nullptr && (legoGlobs.flags2 & GAME2_MENU_HASPREVIOUS) &&
	                       (Gods98::msx() >= prevPos.x && Gods98::msx() < prevPos.x + (sint32)Gods98::Image_GetWidth(legoGlobs.RepeatButtonImage)) &&
	                       (Gods98::msy() >= prevPos.y && Gods98::msy() < prevPos.y + (sint32)Gods98::Image_GetHeight(legoGlobs.RepeatButtonImage)));

	// Press detection for last page:
	// Pressing space or clicking anywhere (except over the previous button) will act as the end of the message.
	/// FIX APPLY: Change space (NextPage shortcut) behaviour to ignore overPrev.
	if (objectiveGlobs.currentPages[briefIdx] == objectiveGlobs.pageCounts[briefIdx]) {
		if (nextKeyPressed || (!overPrev && leftButtonReleased)) {

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {

				Objective_StopShowing();
				Lego_SetMenuNextPosition(nullptr);
				Lego_SetMenuPreviousPosition(nullptr);
				Lego_SetPaused(false, false);

				// Start the level by centering the cursor in the middle of the screen.
				Gods98::Input_SetCursorPos((Gods98::appWidth() / 2), (Gods98::appHeight() / 2));

				// Start the level with object huds off.
				/// TODO: Where is the best place to actually handle this??
				if (Bubble_GetObjectUIsAlwaysVisible()) {
					Bubble_ToggleObjectUIsAlwaysVisible();
				}
			}
			else if (objectiveGlobs.flags & (OBJECTIVE_GLOB_FLAG_COMPLETED|OBJECTIVE_GLOB_FLAG_FAILED)) {

				// Check if all teleporting units have been beamed up.
				if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
					Objective_StopShowing();

					// Exit game if there's no next level/user selects quit from the front end.
					*exitGame = !Lego_EndLevel();
				}
			}
		}
	}
	
	// Press detection for 'next page' button:
	if (objectiveGlobs.currentPages[briefIdx] < objectiveGlobs.pageCounts[briefIdx]) {
		bool nextButtonPressed = false;
		if (overNext) {
			ToolTip_Activate(ToolTip_More);
			if (leftButtonReleased) {
				Lego_SetPointerSFX(PointerSFX_Okay);
				nextButtonPressed = true;
			}
		}
		if (nextKeyPressed || nextButtonPressed) {
			objectiveGlobs.currentPages[briefIdx]++;
		}
	}

	// Press detection for 'previous page' button:
	if (objectiveGlobs.currentPages[briefIdx] > 0) {
		if (overPrev) {
			ToolTip_Activate(ToolTip_Back);
			if (leftButtonReleased) {
				Lego_SetPointerSFX(PointerSFX_Okay);
				objectiveGlobs.currentPages[briefIdx]--;
			}
		}
	}

	return Objective_IsShowing();
}

// <LegoRR.exe @00458ea0>
void __cdecl LegoRR::Objective_Update(Gods98::TextWindow* textWnd, Lego_Level* level, real32 elapsedGame, real32 elapsedAbs)
{
	LevelStatus briefIdx = LEVELSTATUS_FAILED_OTHER; // not showing briefing or completed/failure messages.
	bool stopMusic = false; // Set to true for completed/failure messages only.
	bool flushSFXQueue = false; // Set to true when briefing message only.

	if (Gods98::Main_ProgrammerMode() > 3) {
		Objective_StopShowing(); // Higher programmer modes disable intro/outro briefing entirely.
	}

	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BRIEFING) {
		briefIdx = LEVELSTATUS_INCOMPLETE; // LEVELSTATUS_INCOMPLETE;
		flushSFXQueue = true;

		Lego_SetPaused(false, true);
		
		if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
			objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR) {
				Advisor_Start(Advisor_Objective, true);
			}
		}
		Text_DisplayMessage(Text_SpaceToContinue, false, false);
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_COMPLETED) {
		if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
			briefIdx = LEVELSTATUS_COMPLETE;
			stopMusic = true;

			ObjectRecall_Save_CopyToNewObjectRecallData();
			Lego_SetPaused(false, true);
			if (!level->objective.achievedVideoPlayed) {
				const Point2F* optVideoPos = nullptr;
				if (!level->objective.noAchievedVideoPosition) {
					optVideoPos = &level->objective.achievedVideoPosition;
				}
				Lego_PlayMovie_old(level->objective.achievedVideoName, optVideoPos);
				level->objective.achievedVideoPlayed = true;
			}

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
				objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
				if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR) {
					Advisor_Start(Advisor_Objective, true);
				}
			}
			Text_DisplayMessage(Text_SpaceToContinue, false, false);
		}
	}
	else if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_FAILED) {
		if (Teleporter_ServiceAll(OBJECT_TYPE_FLAGS_TELEPORTED)) {
			briefIdx = (!(objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL) ? LEVELSTATUS_FAILED
																			  : LEVELSTATUS_FAILED_CRYSTALS);
			stopMusic = true;

			ObjectRecall_Save_CreateNewObjectRecall();
			Lego_SetPaused(false, true);

			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_STATUSREADY) {
				objectiveGlobs.flags &= ~OBJECTIVE_GLOB_FLAG_STATUSREADY;
				if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWFAILEDADVISOR) {
					Advisor_Start(Advisor_Objective, true);
				}
			}
			Text_DisplayMessage(Text_SpaceToContinue, false, false);
		}
	}
	else {
		bool32 timerStillRunning;
		if (Objective_CheckCompleted(level, &timerStillRunning, elapsedGame)) {
			Objective_SetStatus(timerStillRunning ? LEVELSTATUS_COMPLETE : LEVELSTATUS_FAILED);
		}
		else {
			RewardQuota_UpdateTimers(elapsedGame);
		}
	}

	if (objectiveGlobs.soundHandle != -1) {
		objectiveGlobs.soundTimer -= elapsedAbs;
		if (objectiveGlobs.soundTimer <= 0.0f) {
			Advisor_End();
			objectiveGlobs.soundHandle = -1;
		}
	}

	if (stopMusic) {
		/// CHANGE: No longer check the GAME2_MUSICREADY flag, since that's only necessary to start playing.
		Lego_ChangeMusicPlaying(false); // End music.
	}

	// Old showing state.
	if (objectiveGlobs.showing && objectiveGlobs.soundName != nullptr) {

		SFX_ID sfxID; // dummy init
		if (SFX_GetType(objectiveGlobs.soundName, &sfxID)) {
			// Unused function call.
			//SFX_IsQueueMode();

			SFX_SetQueueMode(false, false);
			objectiveGlobs.soundHandle = SFX_Random_PlaySoundNormal(sfxID, false);
			const real32 samplePlayTime = SFX_Random_GetSamplePlayTime(sfxID);

			objectiveGlobs.soundTimer = ((samplePlayTime - 1.5f) * STANDARD_FRAMERATE);
			SFX_SetQueueMode_AndFlush(true);
		}
		Gods98::Mem_Free(objectiveGlobs.soundName);
		objectiveGlobs.soundName = nullptr;
	}

	if (flushSFXQueue) {
		SFX_SetQueueMode_AndFlush(true);
	}

	objectiveGlobs.showing = (briefIdx != LEVELSTATUS_FAILED_OTHER);

	if (objectiveGlobs.showing) {
		/// HARDCODED: UI button dimensions/positions for briefing.
		const Point2F nextPos = Point2F { 470.0f, 315.0f };
		const Point2F prevPos = Point2F { 130.0f, 315.0f };

		// Show or hide the 'next page' button.
		if (objectiveGlobs.currentPages[briefIdx] < objectiveGlobs.pageCounts[briefIdx])
			Lego_SetMenuNextPosition(&nextPos);
		else
			Lego_SetMenuNextPosition(nullptr);

		// Show or hide the 'previous page' button.
		if (objectiveGlobs.currentPages[briefIdx] > 0)
			Lego_SetMenuPreviousPosition(&prevPos);
		else
			Lego_SetMenuPreviousPosition(nullptr);

		if (objectiveGlobs.currentPages[briefIdx] != objectiveGlobs.currentPageStates[briefIdx]) {
			Gods98::TextWindow_Clear(objectiveGlobs.textWindows[briefIdx]);
			Gods98::TextWindow_PagePrintF(objectiveGlobs.textWindows[briefIdx], objectiveGlobs.currentPages[briefIdx], "%s", objectiveGlobs.messages[briefIdx]);
			objectiveGlobs.currentPageStates[briefIdx] = objectiveGlobs.currentPages[briefIdx];
		}

		if (level->objective.panelImage != nullptr) {
			Gods98::Image_Display(level->objective.panelImage, &level->objective.panelImagePosition);
		}

		// NOTE: titleTextWindows does not have an index for LEVELSTATUS_FAILED_CRYSTALS.
		/// FIX APPLY: Show Failed title for Failed Crystals status.
		Gods98::TextWindow* titleTextWindow = nullptr;
		switch (level->status) {
		case LEVELSTATUS_INCOMPLETE:
			titleTextWindow = objectiveGlobs.briefingTitleTextWindow;
			break;
		case LEVELSTATUS_COMPLETE:
			titleTextWindow = objectiveGlobs.completedTitleTextWindow;
			break;
		case LEVELSTATUS_FAILED:
		case LEVELSTATUS_FAILED_CRYSTALS:
			titleTextWindow = objectiveGlobs.failedTitleTextWindow;
			break;
		}

		if (titleTextWindow != nullptr) {
			Gods98::TextWindow_Update(titleTextWindow, 0, elapsedGame, nullptr);
		}
		if (objectiveGlobs.textWindows[briefIdx] != nullptr) {
			Gods98::TextWindow_Update(objectiveGlobs.textWindows[briefIdx], 0, elapsedGame, nullptr);
		}
		if (objectiveGlobs.beginTextWindow != nullptr) {
			Gods98::TextWindow_Update(objectiveGlobs.beginTextWindow, 0, elapsedGame, nullptr);
		}
	}
}

// timerStillRunning is set to false when time has run out.
// <LegoRR.exe @00459310>
bool32 __cdecl LegoRR::Objective_CheckCompleted(Lego_Level* level, OUT bool32* timerStillRunning, real32 elapsed)
{
	// No briefing == sandbox or something... WHAT???
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWBRIEFINGADVISOR) {
		return false;
	}

	// Check timer objective:
	*timerStillRunning = true;
	if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_TIMER) {
		level->objective.timer -= elapsed;
		if (level->objective.timer <= 0.0f) {
			/// TODO: Note that this flag serves two purposes, whether its intentional is unclear.
			/// FIX APPLY: Switch to HITTIMEFAIL flag.
			//if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_SHOWACHIEVEDADVISOR) {
			if (objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_HITTIMEFAIL) {
				*timerStillRunning = false;
			}
			return true;
		}
	}

	// Check crystals and ore objectives:
	if ((objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CRYSTAL) &&
		static_cast<uint32>(level->crystals) < level->objective.crystals)
	{
		return false;
	}
	if ((objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_ORE) &&
		static_cast<uint32>(level->ore) < level->objective.ore)
	{
		return false;
	}

	// Check construction or at-block objectives:
	for (auto obj : objectListSet.EnumerateSkipUpgradeParts()) {
		if (Objective_Callback_CheckCompletedObject(obj, &level->objective))
			return true;
	}
	return false;
	//return LegoObject_RunThroughListsSkipUpgradeParts(Objective_Callback_CheckCompletedObject, &level->objective);
}

// DATA: ObjectiveData* objective
// <LegoRR.exe @004593c0>
bool32 __cdecl LegoRR::Objective_Callback_CheckCompletedObject(LegoObject* liveObj, void* pObjective)
{
	const ObjectiveData* objective = static_cast<ObjectiveData*>(pObjective);

	if ((objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_BLOCK) &&
		(liveObj->type == LegoObject_MiniFigure || liveObj->type == LegoObject_Vehicle))
	{
		/// SANITY: Check return value for success.
		sint32 bx, by;
		if (LegoObject_GetBlockPos(liveObj, &bx, &by) &&
			objective->blockPos.x == bx && objective->blockPos.y == by)
		{
			return true;
		}
	}
	if ((objectiveGlobs.flags & OBJECTIVE_GLOB_FLAG_CONSTRUCTION) &&
		objective->constructionType == liveObj->type && objective->constructionID == liveObj->id)
	{
		return true;
	}
	return false;
}

#pragma endregion
