/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as 
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>. 
 * 
 */

#ifndef EDITOR_COMMANDS_H
#define EDITOR_COMMANDS_H

#include <memory>
#include <string>
#include <vector>

#include "rs-kernel/model/map.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/weapon.h"
#include "objective_dialog.h"

namespace rs_win_editor {

/*!
 * Abstract base class for all undoable/redoable editor commands.
 */
class IEditorCommand {
public:
    virtual ~IEditorCommand() = default;
    virtual bool execute() = 0;
    virtual bool undo() = 0;
    virtual std::string description() const = 0;
};

/*!
 * Command to paint a single tile at (tx, ty, tz).
 */
class PaintTileCommand : public IEditorCommand {
public:
    PaintTileCommand(rs_knl::Map *pMap, int tx, int ty, int tz, uint8_t oldTile, uint8_t newTile);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Map *pMap_;
    int tx_, ty_, tz_;
    uint8_t oldTile_;
    uint8_t newTile_;
};

/*!
 * Command to box-fill a 2D rectangular area of tiles at tz.
 */
class BoxFillTilesCommand : public IEditorCommand {
public:
    struct TileRestoreInfo {
        int tx, ty, tz;
        uint8_t tileId;
    };

    BoxFillTilesCommand(rs_knl::Map *pMap, int x1, int y1, int x2, int y2, int tz, uint8_t newTile);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Map *pMap_;
    int minX_, minY_, maxX_, maxY_, tz_;
    uint8_t newTile_;
    std::vector<TileRestoreInfo> previousTiles_;
};

/*!
 * Command to move or copy a tile from (srcX, srcY, srcZ) to (dstX, dstY, dstZ).
 */
class MoveTileCommand : public IEditorCommand {
public:
    MoveTileCommand(rs_knl::Map *pMap, int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, bool copyOnly = false);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Map *pMap_;
    int srcX_, srcY_, srcZ_;
    int dstX_, dstY_, dstZ_;
    bool copyOnly_;
    uint8_t srcTileId_{0};
    uint8_t dstOldTileId_{0};
};

/*!
 * Command to spawn a pedestrian actor.
 */
class SpawnPedCommand : public IEditorCommand {
public:
    SpawnPedCommand(rs_knl::Mission *pMission, rs_knl::PedInstance::PedType type,
                    const rs_knl::TilePoint &pos, uint8_t orientation, uint16_t baseAnim = 0,
                    bool cyberwareOverridden = false, int cyberwareTier = 0,
                    uint8_t movementMode = 0x10, int factionId = -1,
                    bool isOurAgent = false, int squadSlot = -1);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

    rs_knl::PedInstance* getSpawnedPed() const { return pSpawnedPed_; }

private:
    rs_knl::Mission *pMission_;
    rs_knl::PedInstance::PedType type_;
    rs_knl::TilePoint pos_;
    uint8_t orientation_;
    uint16_t baseAnim_{0};
    bool cyberwareOverridden_{false};
    int cyberwareTier_{0};
    uint8_t movementMode_{0x10};
    int factionId_{-1};
    bool isOurAgent_{false};
    int squadSlot_{-1};
    rs_knl::PedInstance *pSpawnedPed_{nullptr};
};

/*!
 * Command to delete a pedestrian actor.
 */
class DeletePedCommand : public IEditorCommand {
public:
    DeletePedCommand(rs_knl::Mission *pMission, rs_knl::PedInstance *pPed);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::PedInstance *pPedRef_;
    rs_knl::PedInstance::PedType type_;
    bool isOurAgent_{false};
    size_t squadSlot_{0};
    rs_knl::TilePoint pos_;
    uint8_t orientation_;
    int health_;
    uint16_t baseAnim_{0};
    std::vector<rs_knl::ScriptStep> scriptSteps_;
    std::vector<rs_knl::Weapon::WeaponType> weapons_;
};

/*!
 * Command to move an entity on the map.
 */
class MoveEntityCommand : public IEditorCommand {
public:
    MoveEntityCommand(rs_knl::MapObject *pObj, const rs_knl::TilePoint &oldPos, const rs_knl::TilePoint &newPos, rs_knl::Mission *pMission = nullptr);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::MapObject *pObj_;
    rs_knl::TilePoint oldPos_;
    rs_knl::TilePoint newPos_;
    rs_knl::Mission *pMission_;
};

/*!
 * Command to spawn a vehicle on the map.
 */
class SpawnVehicleCommand : public IEditorCommand {
public:
    SpawnVehicleCommand(rs_knl::Mission *pMission, uint8_t type,
                        const rs_knl::TilePoint &pos, uint8_t orientation,
                        int health = 0, int driverType = 0, uint16_t driverBaseAnim = 0,
                        uint16_t baseAnim = 0);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

    rs_knl::Vehicle* getSpawnedVehicle() const { return pSpawnedVehicle_; }
    rs_knl::PedInstance* getSpawnedDriver() const { return pSpawnedDriver_; }

private:
    rs_knl::Mission *pMission_;
    uint8_t type_;
    rs_knl::TilePoint pos_;
    uint8_t orientation_;
    int health_{0};
    int driverType_{0};
    uint16_t driverBaseAnim_{0};
    uint16_t baseAnim_{0};
    rs_knl::Vehicle *pSpawnedVehicle_{nullptr};
    std::vector<rs_knl::Vehicle*> spawnedVehicles_;
    rs_knl::PedInstance *pSpawnedDriver_{nullptr};
};

/*!
 * Command to delete a vehicle.
 */
class DeleteVehicleCommand : public IEditorCommand {
public:
    DeleteVehicleCommand(rs_knl::Mission *pMission, rs_knl::Vehicle *pVehicle);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::Vehicle *pVehicleRef_;
    uint8_t type_;
    rs_knl::TilePoint pos_;
    uint8_t orientation_;
    int health_{0};
    uint16_t baseAnim_{0};
    int origVehicleIndex_{-1};
    rs_knl::PedInstance *origDriver_{nullptr};
    std::vector<std::pair<rs_knl::PedInstance*, std::vector<rs_knl::ScriptStep>>> affectedPedRoutes_;
};

/*!
 * Command to spawn a static prop on the map.
 */
class SpawnStaticCommand : public IEditorCommand {
public:
    SpawnStaticCommand(rs_knl::Mission *pMission, uint16_t staticId, const rs_knl::TilePoint &pos, uint16_t baseAnim = 0, uint8_t orientation = 0);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

    rs_knl::Static* getSpawnedStatic() const { return pSpawnedStatic_; }

private:
    rs_knl::Mission *pMission_;
    uint16_t staticId_;
    rs_knl::TilePoint pos_;
    uint16_t baseAnim_{0};
    uint8_t orientation_{0};
    rs_knl::Static *pSpawnedStatic_{nullptr};
};

/*!
 * Command to delete a static prop.
 */
class DeleteStaticCommand : public IEditorCommand {
public:
    DeleteStaticCommand(rs_knl::Mission *pMission, rs_knl::Static *pStatic);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::Static *pStaticRef_;
    uint16_t staticId_;
    uint16_t baseAnim_{0};
    uint8_t orientation_{0};
    rs_knl::TilePoint pos_;
};

/*!
 * Command to spawn a ground weapon drop.
 */
class SpawnWeaponCommand : public IEditorCommand {
public:
    SpawnWeaponCommand(rs_knl::Mission *pMission, rs_knl::Weapon::WeaponType wType, const rs_knl::TilePoint &pos);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::Weapon::WeaponType wType_;
    rs_knl::TilePoint pos_;
    rs_knl::WeaponInstance *pSpawnedWeapon_{nullptr};
};

/*!
 * Command to delete a ground weapon drop.
 */
class DeleteWeaponCommand : public IEditorCommand {
public:
    DeleteWeaponCommand(rs_knl::Mission *pMission, rs_knl::WeaponInstance *pWeapon);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::WeaponInstance *pWeaponRef_;
    rs_knl::Weapon::WeaponType wType_;
    rs_knl::TilePoint pos_;
};

/*!
 * Command to modify a pedestrian actor's scripted action route (waypoints, triggers, loops).
 */
class ModifyPedRouteCommand : public IEditorCommand {
public:
    ModifyPedRouteCommand(rs_knl::Mission *pMission, rs_knl::PedInstance *pPed,
                          const std::vector<rs_knl::ScriptStep> &oldSteps,
                          const std::vector<rs_knl::ScriptStep> &newSteps,
                          bool oldLoop = false, bool newLoop = false);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    rs_knl::PedInstance *pPed_;
    std::vector<rs_knl::ScriptStep> oldSteps_;
    std::vector<rs_knl::ScriptStep> newSteps_;
    bool oldLoop_ = false;
    bool newLoop_ = false;
};

/*!
 * Command to modify entity attributes (health, orientation/direction, panic, weapons, driver).
 */
class ModifyEntityAttributesCommand : public IEditorCommand {
public:
    struct Attrs {
        int health{100};
        uint8_t direction{0};
        std::vector<rs_knl::Weapon::WeaponType> weapons;
        rs_knl::PedInstance *pDriver{nullptr};
        int driverChoice{0};
        bool cyberwareOverridden{false};
        int cyberwareTier{0};
        uint8_t movementMode{0x10};
        int factionId{-1};
    };

    ModifyEntityAttributesCommand(rs_knl::Mission *pMission,
                                  rs_knl::ShootableMapObject *pObj,
                                  const Attrs &oldAttrs,
                                  const Attrs &newAttrs);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    void apply(const Attrs &attrs, bool isUndo);

    rs_knl::Mission *pMission_;
    rs_knl::ShootableMapObject *pObj_;
    Attrs oldAttrs_;
    Attrs newAttrs_;
    rs_knl::PedInstance *pSpawnedDriver_{nullptr};
    rs_knl::TilePoint oldDriverExitPos_;
    rs_knl::PedInstance *pPreviousDriver_{nullptr};
    std::vector<rs_knl::ScriptStep> oldDriverRoute_;
    std::vector<rs_knl::ScriptStep> newDriverPreviousRoute_;
};

/*!
 * Command to add a mission objective.
 */
class AddObjectiveCommand : public IEditorCommand {
public:
    AddObjectiveCommand(rs_knl::Mission *pMission, const ObjectiveDialogData &data);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    ObjectiveDialogData data_;
};

/*!
 * Command to edit an existing mission objective.
 */
class EditObjectiveCommand : public IEditorCommand {
public:
    EditObjectiveCommand(rs_knl::Mission *pMission, size_t index,
                         const ObjectiveDialogData &oldData,
                         const ObjectiveDialogData &newData);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    size_t index_;
    ObjectiveDialogData oldData_;
    ObjectiveDialogData newData_;
};

/*!
 * Command to delete a mission objective.
 */
class DeleteObjectiveCommand : public IEditorCommand {
public:
    DeleteObjectiveCommand(rs_knl::Mission *pMission, size_t index,
                           const ObjectiveDialogData &oldData);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    size_t index_;
    ObjectiveDialogData oldData_;
};

/*!
 * Command to clear all mission objectives.
 */
class ClearObjectivesCommand : public IEditorCommand {
public:
    ClearObjectivesCommand(rs_knl::Mission *pMission,
                           const std::vector<ObjectiveDialogData> &oldDataList);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    std::vector<ObjectiveDialogData> oldDataList_;
};

/*!
 * Command to edit AI tactics block.
 */
class EditAITacticsCommand : public IEditorCommand {
public:
    EditAITacticsCommand(rs_knl::Mission *pMission,
                         const LevelData::AITacticsBlock &oldTactics,
                         const LevelData::AITacticsBlock &newTactics);
    bool execute() override;
    bool undo() override;
    std::string description() const override;

private:
    rs_knl::Mission *pMission_;
    LevelData::AITacticsBlock oldTactics_;
    LevelData::AITacticsBlock newTactics_;
};

/*!
 * Command stack managing Undo and Redo histories.
 */
class EditorCommandStack {
public:
    explicit EditorCommandStack(size_t maxHistory = 50);

    bool pushAndExecute(std::unique_ptr<IEditorCommand> cmd);
    bool executeCommand(std::unique_ptr<IEditorCommand> cmd) { return pushAndExecute(std::move(cmd)); }
    bool undo();
    bool redo();

    bool canUndo() const { return !undoStack_.empty(); }
    bool canRedo() const { return !redoStack_.empty(); }
    size_t undoCount() const { return undoStack_.size(); }

    void clear();

    std::string getUndoDescription() const;
    std::string getRedoDescription() const;
    std::string lastUndoDescription() const { return getUndoDescription(); }
    std::string lastRedoDescription() const { return getRedoDescription(); }

private:
    size_t maxHistory_;
    std::vector<std::unique_ptr<IEditorCommand>> undoStack_;
    std::vector<std::unique_ptr<IEditorCommand>> redoStack_;
};

} // namespace rs_win_editor

#endif // EDITOR_COMMANDS_H
