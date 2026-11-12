/* Current selected original MG42 alias playback. Ordinary imports unchanged.
 * Own invocation fields are copied into the actual loop/channel, never stored
 * by WAV handle (two aliases may share a WAV with different caps). */
void S_Cod1AliasSlave(const struct cod1AliasSlaveRequest_s *input){cod1AliasSlaveRequest r;int i,before;
 if(!input)return;r=*input;if((r.looping!=0&&r.looping!=1)||r.entity<0||r.entity>=MAX_GENTITIES||r.channel<0||r.channel>CHAN_ANNOUNCER||r.handle<=0||r.handle>=snd.s_numSfx||!CQ_Finite(r.slave_cap)||r.slave_cap<0)return;
 for(i=0;i<3;i++)if(!CQ_Finite(r.origin[i])||!CQ_Finite(r.velocity[i]))return;
 if(r.looping){if(r.range<=0||r.volume<0||r.volume>255)return;before=snd.numLoopSounds;S_AddLoopingSound(r.entity,r.origin,r.velocity,r.range,r.handle,r.volume);if(snd.numLoopSounds==before+1&&snd.loopSounds[before].sfx==&s_knownSfx[r.handle])snd.loopSounds[before].cod1SlaveCap=r.slave_cap;}
 else {if(!CQ_Finite(r.gain)||r.gain<0||r.gain>2||!CQ_Finite(r.pitch)||r.pitch<=0||r.pitch>4||!CQ_Finite(r.dist_min)||r.dist_min<0||!CQ_Finite(r.dist_max)||(r.dist_max!=0&&r.dist_max<r.dist_min))return;if(!snd.s_soundStarted||snd.s_soundMute||cls.state!=CA_ACTIVE)return;S_ThreadStartSoundExParams(r.origin,r.entity,r.channel,r.handle,0,r.gain,r.pitch,r.dist_min,r.dist_max,qtrue,r.slave_cap);}
}
