#!/usr/bin/env python3
"""
Practical migration helper for defineEditorAttribute refactoring.

This tool provides:
1. List of files grouped by migration complexity
2. Code templates for common patterns
3. Migration checklist
"""

import os
import re
from pathlib import Path
from collections import defaultdict


def scan_files(base_path):
    """Scan for files with defineEditorAttribute."""
    results = {
        'slider_simple': [],  # Static slider values (0-360, etc.)
        'slider_dynamic': [],  # Dynamic slider values (grid-dependent)
        'button_text': [],  # Dynamic button text
        'complex': [],  # Complex attributes (QFont, arrays, etc.)
        'already_migrated': []  # Files without defineEditorAttribute
    }

    for root, dirs, files in os.walk(base_path):
        if 'build' in root or 'ThirdParty' in root or '.git' in root:
            continue

        for filename in files:
            if not filename.endswith('.cpp'):
                continue

            filepath = os.path.join(root, filename)

            try:
                with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                    content = f.read()

                if 'defineEditorAttribute' not in content:
                    continue

                # Check what type of migration this needs
                if 'QFont' in content or 'pickEventHandler' in content:
                    results['complex'].append((filepath, 'Contains QFont or event handlers'))
                elif 'columnWidths' in content:
                    results['complex'].append((filepath, 'Contains array attributes'))
                elif 'PdmUiDoubleSliderEditorAttribute' in content or 'PdmUiSliderEditorAttribute' in content:
                    # Check if slider values are static or dynamic
                    if re.search(r'm_minimum\s*=\s*\d+', content) or re.search(r'm_minimum\s*=\s*0\.0', content):
                        results['slider_simple'].append(filepath)
                    else:
                        results['slider_dynamic'].append(filepath)
                elif 'PdmUiPushButtonEditorAttribute' in content and 'm_buttonText' in content:
                    results['button_text'].append(filepath)
                else:
                    results['complex'].append((filepath, 'Unknown pattern'))

            except Exception as e:
                print(f"Error reading {filepath}: {e}")

    return results


def print_templates():
    """Print migration code templates."""
    print("\n" + "=" * 80)
    print("MIGRATION CODE TEMPLATES")
    print("=" * 80)

    print("""
1. STATIC SLIDER ATTRIBUTES (e.g., angle 0-360):
   -----------------------------------------------
   In constructor (if truly static):
       m_azimuthAngle.uiCapability()->setAttributeInt( "minimum", 0 );
       m_azimuthAngle.uiCapability()->setAttributeInt( "maximum", 360 );
       m_azimuthAngle.uiCapability()->setAttributeInt( "sliderTickCount", 360 );

   Or in defineUiOrdering (if needs to be set on each UI refresh):
       m_azimuthAngle.uiCapability()->setAttributeInt( "minimum", 0 );
       m_azimuthAngle.uiCapability()->setAttributeInt( "maximum", 360 );
       m_azimuthAngle.uiCapability()->setAttributeInt( "sliderTickCount", 360 );

2. DYNAMIC SLIDER ATTRIBUTES (based on runtime data):
   ---------------------------------------------------
   In defineUiOrdering():
       auto wellPath = firstAncestorOrThisOfType<RimWellPath>();
       if ( wellPath )
       {
           m_measuredDepth.uiCapability()->setAttributeDouble( "minimum", wellPath->startMD() );
           m_measuredDepth.uiCapability()->setAttributeDouble( "maximum", wellPath->endMD() );
       }

3. DYNAMIC BUTTON TEXT:
   --------------------
   In defineUiOrdering():
       if ( m_show3DManipulator )
       {
           m_show3DManipulator.uiCapability()->setAttributeString( "buttonText", "Hide 3D manipulator" );
       }
       else
       {
           m_show3DManipulator.uiCapability()->setAttributeString( "buttonText", "Show 3D manipulator" );
       }

4. REMOVING defineEditorAttribute:
   -------------------------------
   - Remove method implementation from .cpp file
   - Remove method declaration from .h file
   - Ensure all attributes are migrated to constructor or defineUiOrdering

ATTRIBUTE NAME MAPPING:
----------------------
Old (defineEditorAttribute)     -> New (setAttribute*)
m_minimum                       -> "minimum"
m_maximum                       -> "maximum"
m_sliderTickCount               -> "sliderTickCount"
m_buttonText                    -> "buttonText"
m_placeholderText               -> "placeholderText"
maximumWidth                    -> "maximumWidth"

SETTER METHODS:
--------------
setAttributeInt( name, value )     - for int values
setAttributeDouble( name, value )  - for double values
setAttributeString( name, value )  - for string values
setAttributeBool( name, value )    - for bool values
""")


def print_report(results):
    """Print migration report."""
    print("\n" + "=" * 80)
    print("MIGRATION STATUS REPORT")
    print("=" * 80)

    total = sum(len(v) if isinstance(v, list) else len(v) for v in results.values())

    print(f"\nTotal files analyzed: {total}")
    print(f"  Simple sliders (static values): {len(results['slider_simple'])}")
    print(f"  Dynamic sliders (runtime values): {len(results['slider_dynamic'])}")
    print(f"  Button text: {len(results['button_text'])}")
    print(f"  Complex (manual review): {len(results['complex'])}")

    print("\n" + "=" * 80)
    print("SIMPLE SLIDER FILES (Easy - Static Values)")
    print("=" * 80)
    for filepath in sorted(results['slider_simple'])[:20]:  # Show first 20
        print(f"  {filepath}")
    if len(results['slider_simple']) > 20:
        print(f"  ... and {len(results['slider_simple']) - 20} more")

    print("\n" + "=" * 80)
    print("DYNAMIC SLIDER FILES (Medium - Runtime Values)")
    print("=" * 80)
    for filepath in sorted(results['slider_dynamic'])[:20]:
        print(f"  {filepath}")
    if len(results['slider_dynamic']) > 20:
        print(f"  ... and {len(results['slider_dynamic']) - 20} more")

    print("\n" + "=" * 80)
    print("BUTTON TEXT FILES (Easy)")
    print("=" * 80)
    for filepath in sorted(results['button_text'])[:20]:
        print(f"  {filepath}")
    if len(results['button_text']) > 20:
        print(f"  ... and {len(results['button_text']) - 20} more")

    print("\n" + "=" * 80)
    print("COMPLEX FILES (Manual Review Required)")
    print("=" * 80)
    for filepath, reason in sorted(results['complex'])[:20]:
        print(f"  {filepath}")
        print(f"    Reason: {reason}")
    if len(results['complex']) > 20:
        print(f"  ... and {len(results['complex']) - 20} more")


def generate_migration_script():
    """Generate a bash/batch script to help with migrations."""
    script = """#!/bin/bash
# Migration helper script
# This script helps track and execute migrations

MIGRATED_FILES=(
    # Add files here as you complete them
)

# Function to check if file is migrated
is_migrated() {
    local file=$1
    for migrated in "${MIGRATED_FILES[@]}"; do
        if [[ "$file" == "$migrated" ]]; then
            return 0
        fi
    done
    return 1
}

# Show progress
echo "Migration Progress:"
echo "Completed: ${#MIGRATED_FILES[@]} files"
"""

    with open('migration_progress.sh', 'w') as f:
        f.write(script)

    print("\nGenerated migration_progress.sh to track progress")


def main():
    base_path = 'ApplicationLibCode' if os.path.exists('ApplicationLibCode') else '.'

    print(f"Scanning {base_path}...")
    results = scan_files(base_path)

    print_report(results)
    print_templates()

    print("\n" + "=" * 80)
    print("RECOMMENDED APPROACH")
    print("=" * 80)
    print("""
1. Start with 'Simple Slider Files' - these have static values (0-360, etc.)
   - Easiest to migrate
   - Low risk

2. Move to 'Button Text Files'
   - Usually straightforward if-else logic
   - Medium risk

3. Then 'Dynamic Slider Files'
   - Need to understand runtime dependencies
   - Medium-high risk

4. Finally 'Complex Files'
   - Require manual analysis
   - Keep defineEditorAttribute for truly complex cases

WORKFLOW:
1. Pick a file from the list above
2. Open the .cpp file and find defineEditorAttribute implementation
3. Identify the pattern (use templates above)
4. Add attribute calls to defineUiOrdering() or constructor
5. Remove defineEditorAttribute method from .h and .cpp
6. Build and test
7. Commit

TIP: Do files in batches of 5-10, commit after each batch.
""")


if __name__ == '__main__':
    main()
