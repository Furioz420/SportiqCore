/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This is where scripts' loading functions should be declared:
// void MyExampleScript()
void AddSC_spell_custom_grievous();           // handles 3001000 DOT
void AddSC_spell_custom_raging();
void AddSC_spell_custom_storming();
void AddSC_spell_custom_volcanic();
void AddSC_spell_custom_bolstering();
void AddSC_spell_custom_sanguine();
void AddSC_spell_custom_explosive();
void AddSC_spell_custom_quaking();
void AddSC_spell_custom_spiteful();
void AddSC_spell_custom_entangling();
void AddSC_npc_entangling_plant();
void AddSC_spell_custom_inspiring();
void AddSC_spell_custom_shielding();
void AddSC_npc_shielding_orb();
void AddSC_npc_manifestation_of_pride();
void AddSC_spell_belligerent_missile_launcher();
void AddSC_npc_belligerent_missile();
void AddSC_spell_bursting_with_pride();

// The name of this function should match:
// void Add${NameOfDirectory}Scripts()
void AddCustomScripts()
{
    // MyExampleScript()
    AddSC_spell_custom_grievous();           // handles 3001000 DOT
    AddSC_spell_custom_raging();
    AddSC_spell_custom_storming();
    AddSC_spell_custom_volcanic();
    AddSC_spell_custom_bolstering();
    AddSC_spell_custom_sanguine();
    AddSC_spell_custom_explosive();
    AddSC_spell_custom_quaking();
    AddSC_spell_custom_spiteful();
    AddSC_spell_custom_entangling();
    AddSC_npc_entangling_plant();
    AddSC_spell_custom_inspiring();
    AddSC_spell_custom_shielding();
    AddSC_npc_shielding_orb();
    AddSC_npc_manifestation_of_pride();
    AddSC_spell_belligerent_missile_launcher();
    AddSC_npc_belligerent_missile();
    AddSC_spell_bursting_with_pride();
}
