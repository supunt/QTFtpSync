# QTFtpSync
This is  FTP tool made to sync a local folder to a remote location.

This is a rough tool put together quickly. Hence it is not the nicest code.
This uses a legacy FTP library in QT 4.2.

This is does some fun stuff.
1) Syncs the folder that is pointed at in a regular interval.
2) Re-try connections and transfers.
3) Writes a log.
4) Writes a activity trail.
5) Backs up evetything trasnferred during the day to a backup foder when the backup time is reached.

And much more that I have forgotten. :D

Settings
1) Sync Interval : in seconds
2) Remote Host : Where the files are transfered to.
3) Remote user : FTP user (Leave empty for anonymous)
4) Remote password : FTP pass (Leave empty for anonymous)
5) Remote path : Folder of depth 1
6) FTP mode : Passive/Active
7) Housekeeping Time : Time when the transferred files are backed up to a backup location for audits.
8) Log Path : Where the log is written
9) Backup Path : Files transferred during the operation hours are put in to this folder when the 'Housekeeping Time' is reached

Limitations.
1) You can select a remote folder to a depth on one only.
