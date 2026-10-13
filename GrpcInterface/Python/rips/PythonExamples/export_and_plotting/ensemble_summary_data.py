###############################################################################
# Read ensemble summary data into a pandas DataFrame, compute a new vector and
# write it back to ResInsight
#
# Requires pandas
###############################################################################

import sys

import rips

resinsight = rips.Instance.find()
project = resinsight.project

ensembles = project.summary_ensembles()

if not ensembles:
    print("No summary ensemble found in the current project")
    sys.exit(1)

ensemble = ensembles[0]

# Columns are DATE, REAL (summary case id) and one column per vector. Use the
# original time steps ("NONE") when the data is to be written back.
df = ensemble.to_dataframe(["FOPT", "FWPT"], time_resolution="NONE")
print(df.head())

# Statistics per date across the realizations
print(df.groupby("DATE")["FOPT"].describe())

# Compute a new vector. Names are limited to 8 characters, and vectors from the
# original summary files cannot be overwritten, so pick a name that is unused.
existing = set()
for summary_case in ensemble.summary_cases():
    existing.update(summary_case.available_addresses().values)

new_name = next(f"FWCT_{i}" for i in range(1000) if f"FWCT_{i}" not in existing)
df[new_name] = df["FWPT"] / (df["FOPT"] + df["FWPT"])
new_data = df[["DATE", "REAL", new_name]]

ensemble.set_vectors_from_dataframe(new_data, units={new_name: ""})

# Vectors from all realizations can also be read without pandas
values = ensemble.vector_values("FOPT", time_resolution="YEAR")
for real, data in values.items():
    print(real, data["values"])
