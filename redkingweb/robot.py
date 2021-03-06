#!/usr/bin/env python
# Red King Simulation Sonification
# Copyright (C) 2016 Foam Kernow
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os
import math
import time
import hashlib
import numpy as np
import scipy.io.wavfile
import robot.redking
import random
from robot.sim_helpers import *
from robot import thumb
from robot import synth
from robot import blip
import robot.twitter

os.environ.setdefault("DJANGO_SETTINGS_MODULE", "redkingweb.settings")

import django
import datetime
from redking.models import *
from django.utils import timezone
from django.db.models import Max, Count, Sum

django.setup()

# time in seconds, step in samples
def render_sim(model,synth,time_length,synth_step):
    sim_length = time_length*44100
    out = np.zeros(sim_length,dtype=np.float32)
    steps = sim_length/synth_step
    synth.t_max=synth_step
    pos = 0
    th = thumb.thumb(steps,model.size(),10)
    for i in range(0,steps):
        for p in range(0,synth_step):
            out[pos]=synth.render()
            pos+=1
        th.render(model)
        synth.update(combined_array(model))
        model.step()
        #time.sleep(1)

    return out,th

# time in seconds, step in samples
def render_blipsim(model,blip,time_length):
    sim_length = int(time_length*44100)
    out = np.zeros(sim_length,dtype=np.float32)
    steps = sim_length/blip.bar_length/2
    skip = 20
    th = thumb.thumb(steps*skip,model.size(),10)

    # check by pre-running for a bit
    pre_run = 100
    for i in range(0,pre_run):
        model.step()
        time.sleep(0.3)

    blip.update(parasite_state_array(model))
    if len(blip.blips)<2: 
        print("only one parasite strain, rejecting")
        return False,False
    #blip.update(host_state_array(model))
    #if len(blip.blips)<2: return False,False
    
    # restart everything
    model.init()
    blip.init()

    for i in range(0,steps):
        blip.update(parasite_state_array(model))
        blip.render(out,"PING")
        blip.update(host_state_array(model))
        blip.render(out,"PING")

        for i in range(0,skip):
            th.render(model)
            model.step()
            if model.is_extinct():
                print("extinct, rejecting")
                return False,False
            time.sleep(0.3)

    return out,th

def update_fitness():
    for s in Sim.objects.all():
        fit = s.upvotes - s.downvotes
        if fit!=s.fitness:
            s.fitness = fit
            s.save()

def cp_from_db(s):
    cp = redking.model_cost_params()
    cp.amin = s.param_amin
    cp.amax = s.param_amax
    cp.a_p = s.param_a_p
    cp.betmin = s.param_bmin
    cp.bemaxtime = s.param_bmax
    cp.beta_p = s.param_beta_p
    cp.g = s.param_g
    cp.h = s.param_h
    # additional..
    cp.pstart = s.param_pstart
    cp.hstart = s.param_hstart
    cp.model_type = s.param_model_type


def get_new_cp():
    update_fitness()
    # new random one
    if random.random()<0.5: 
        print("new random params")
        return False,random_cp()

    # pick an existing one
    q = Sim.objects.order_by('-fitness')
    for i,s in enumerate(q):
        # higher up = more likely
        if random.random()<0.1:
            print("picked: "+str(s.base_name)+" ("+str(i)+")")
            cp = str_to_cp(s.params)
            if cp==False:
                print("old params...")
                return False,random_cp()
            else:
                return s,mutate_cp(cp)

def run(location):
    length = 40
    parent,cp = get_new_cp()
    params_str = cp_to_str(cp)
    base_name = hashlib.md5(params_str).hexdigest()
    m = redking.model()
    m.set_model(cp.model_type)
    m.m_pstart = cp.pstart;
    m.m_hstart = cp.hstart;
    m.m_cost_params=cp

    m.init()
    #s = synth.synth(m.size())
    s = blip.blip(m.size(),0.25)
    #out,th = render_sim(m,s,length,1000)
    out,th = render_blipsim(m,s,length)
    if th!=False:
        wavname = location+base_name+".wav"
        imgname = location+base_name+".png"
        scipy.io.wavfile.write(wavname,44100,out)
        os.system("lame "+wavname)
        #os.system("aplay "+wavname)
        os.system("rm "+wavname)
        th.save(imgname)
        os.system("mogrify -resize 600% -filter Point "+imgname)
        d=Sim(created_date = timezone.now(),
              base_name = base_name,
              length = length,
              params = params_str)
        if parent!=False:
            d.parent = parent
        d.save()
        robot.twitter.tweet("I just generated new host/parasite evolution music: http://redking.fo.am/sim/"+str(d.id),imgname,robot.twitter.api)

while(True):
    run("media/sim/")
